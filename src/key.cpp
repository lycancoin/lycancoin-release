// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "key.h"

#include "arith_uint256.h"
#include "crypto/common.h"
#include "crypto/hmac_sha512.h"
#include "eccryptoverify.h"
#include "pubkey.h"
#include "random.h"

#include <secp256k1.h>
#include "ecwrapper.h"

// anonymous namespace
namespace {

class CSecp256k1Init {
public:
    CSecp256k1Init() {
        secp256k1_start(SECP256K1_START_SIGN);
    }
    ~CSecp256k1Init() {
        secp256k1_stop();
    }
};
static CSecp256k1Init instance_of_csecp256k1;

} // anon namespace

bool CKey::Check(const unsigned char *vch) {
    return eccrypto::Check(vch);
}

void CKey::MakeNewKey(bool fCompressedIn) {
    RandAddSeedPerfmon();
    do {
        GetRandBytes(vch, sizeof(vch));
    } while (!Check(vch));
    fValid = true;
    fCompressed = fCompressedIn;
}

bool CKey::SetPrivKey(const CPrivKey &privkey, bool fCompressedIn) {
    if (!secp256k1_ec_privkey_import((unsigned char*)begin(), &privkey[0], privkey.size()))
        return false;
    fCompressed = fCompressedIn;
    fValid = true;
    return true;
}

CPrivKey CKey::GetPrivKey() const {
    assert(fValid);
    CPrivKey privkey;
    int privkeylen, ret;
    privkey.resize(279);
    privkeylen = 279;
    ret = secp256k1_ec_privkey_export(begin(), (unsigned char*)&privkey[0], &privkeylen, fCompressed);
    assert(ret);
    privkey.resize(privkeylen);
    return privkey;
}

CPubKey CKey::GetPubKey() const {
    assert(fValid);
    CPubKey result;
    int clen = 65;
    int ret = secp256k1_ec_pubkey_create((unsigned char*)result.begin(), &clen, begin(), fCompressed);
    assert((int)result.size() == clen);
    assert(ret);
    assert(result.IsValid());
    return result;
}

bool CKey::Sign(const uint256 &hash, std::vector<unsigned char>& vchSig, uint32_t test_case) const {
    if (!fValid)
        return false;
    vchSig.resize(72);
    int nSigLen = 72;
    unsigned char extra_entropy[32] = {0};
    WriteLE32(extra_entropy, test_case);
    int ret = secp256k1_ecdsa_sign(hash.begin(), (unsigned char*)&vchSig[0], &nSigLen, begin(), secp256k1_nonce_function_rfc6979, test_case ? extra_entropy : NULL);
    assert(ret);
    vchSig.resize(nSigLen);
    return true;
}

bool CKey::VerifyPubKey(const CPubKey& pubkey) const {
    if (pubkey.IsCompressed() != fCompressed) {
        return false;
    }
    unsigned char rnd[8];
    std::string str = "Lycancoin key verification\n";
    GetRandBytes(rnd, sizeof(rnd));
    uint256 hash;
    CHash256().Write((unsigned char*)str.data(), str.size()).Write(rnd, sizeof(rnd)).Finalize(hash.begin());
    std::vector<unsigned char> vchSig;
    Sign(hash, vchSig);
    return pubkey.Verify(hash, vchSig);
}

bool CKey::SignCompact(const uint256 &hash, std::vector<unsigned char>& vchSig) const {
    if (!fValid)
        return false;
    vchSig.resize(65);
    int rec = -1;
    int ret = secp256k1_ecdsa_sign_compact(hash.begin(), &vchSig[1], begin(), secp256k1_nonce_function_rfc6979, NULL, &rec);
    assert(ret);
    assert(rec != -1);
    vchSig[0] = 27 + rec + (fCompressed ? 4 : 0);
    return true;
}

bool CKey::Load(CPrivKey &privkey, CPubKey &vchPubKey, bool fSkipCheck=false) {
    if (!secp256k1_ec_privkey_import((unsigned char*)begin(), &privkey[0], privkey.size()))
        return false;
    fCompressed = vchPubKey.IsCompressed();
    fValid = true;

    if (fSkipCheck)
        return true;

    return VerifyPubKey(vchPubKey);
}

bool CKey::Derive(CKey& keyChild, unsigned char ccChild[32], unsigned int nChild, const unsigned char cc[32]) const {
    assert(IsValid());
    assert(IsCompressed());
    unsigned char out[64];
    LockObject(out);
    if ((nChild >> 31) == 0) {
        CPubKey pubkey = GetPubKey();
        assert(pubkey.begin() + 33 == pubkey.end());
        BIP32Hash(cc, nChild, *pubkey.begin(), pubkey.begin()+1, out);
    } else {
        assert(begin() + 32 == end());
        BIP32Hash(cc, nChild, 0, begin(), out);
    }
    memcpy(ccChild, out+32, 32);
    memcpy((unsigned char*)keyChild.begin(), begin(), 32);
    bool ret = secp256k1_ec_privkey_tweak_add((unsigned char*)keyChild.begin(), out);
    UnlockObject(out);
    keyChild.fCompressed = true;
    keyChild.fValid = ret;
    return ret;
}

bool CExtKey::Derive(CExtKey &out, unsigned int nChild) const {
    out.nDepth = nDepth + 1;
    CKeyID id = key.GetPubKey().GetID();
    memcpy(&out.vchFingerprint[0], &id, 4);
    out.nChild = nChild;
    return key.Derive(out.key, out.vchChainCode, nChild, vchChainCode);
}

void CExtKey::SetMaster(const unsigned char *seed, unsigned int nSeedLen) {
    static const unsigned char hashkey[] = {'B','i','t','c','o','i','n',' ','s','e','e','d'};
    unsigned char out[64];
    LockObject(out);
    CHMAC_SHA512(hashkey, sizeof(hashkey)).Write(seed, nSeedLen).Finalize(out);
    key.Set(&out[0], &out[32], true);
    memcpy(vchChainCode, &out[32], 32);
    UnlockObject(out);
    nDepth = 0;
    nChild = 0;
    memset(vchFingerprint, 0, sizeof(vchFingerprint));
}

CExtPubKey CExtKey::Neuter() const {
    CExtPubKey ret;
    ret.nDepth = nDepth;
    memcpy(&ret.vchFingerprint[0], &vchFingerprint[0], 4);
    ret.nChild = nChild;
    ret.pubkey = key.GetPubKey();
    memcpy(&ret.vchChainCode[0], &vchChainCode[0], 32);
    return ret;
}

void CExtKey::Encode(unsigned char code[74]) const {
    code[0] = nDepth;
    memcpy(code+1, vchFingerprint, 4);
    code[5] = (nChild >> 24) & 0xFF; code[6] = (nChild >> 16) & 0xFF;
    code[7] = (nChild >>  8) & 0xFF; code[8] = (nChild >>  0) & 0xFF;
    memcpy(code+9, vchChainCode, 32);
    code[41] = 0;
    assert(key.size() == 32);
    memcpy(code+42, key.begin(), 32);
}

void CExtKey::Decode(const unsigned char code[74]) {
    nDepth = code[0];
    memcpy(vchFingerprint, code+1, 4);
    nChild = (code[5] << 24) | (code[6] << 16) | (code[7] << 8) | code[8];
    memcpy(vchChainCode, code+9, 32);
    key.Set(code+42, code+74, true);
}

bool ECC_InitSanityCheck() {
    if (!CECKey::SanityCheck()) {
        return false;
    }
    CKey key;
    key.MakeNewKey(true);
    CPubKey pubkey = key.GetPubKey();
    return key.VerifyPubKey(pubkey);
}