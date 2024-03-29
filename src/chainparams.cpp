// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"

#include "util.h"
#include "utilstrencodings.h"

#include "assert.h"

#include <boost/assign/list_of.hpp>

using namespace std;

#include "chainparamsseeds.h"

//
// Main network
//

    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    //
static Checkpoints::MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
        (         0, uint256S("0x50f80e3dea383a355eb15e4be1f122acbc4144bceaa86604555953c5b8a0c9e4"))
        (	  12000, uint256S("0x96c6c3ad38d5f7102d5e14ed730befe4c0865777caac2392ae658dc1b7acc20f"))
        (	  17250, uint256S("0xf4f5d8b8928fe9f3f434b27096eebee85874c4d855a8e6c732c6c34765e32a13"))
        (	  24250, uint256S("0x39170f93af0eee880dbdb25f583feb3f48fe4146bcb7d2fe100bc8708407b680"))
        (	  72000, uint256S("0xc86a831fa0aa3511d746db05427d01fee1ff18192db12817d37d561f9bbfe585"))
        (    140000, uint256S("0xf9d5d85ca836627ba4d4463427182a11e25502b505c91124d587e1284b024d79"))
        (   1380000, uint256S("0x6a3eb7216bf241846f8f370d391673e422ab3352821284fb4b75b8cf0d67071d"))
        (   1415377, uint256S("0xcfa3542981322d81c02a03e0a75569dbe9d85944083c5592ae9efefc27425f38"))
        (   1570900, uint256S("0x8b39517eab83db0718c51489cc15ffa92e54cd0893ad144dbca406f03e85997d"))
        ;

static const Checkpoints::CCheckpointData data = {
        &mapCheckpoints,
        1618027155, // * UNIX timestamp of last checkpoint block
        1683624,   // * total number of transactions between genesis and last checkpoint
                    //   (the tx=... number in the SetBestChain debug.log lines)
        576.0     // * estimated number of transactions per day after checkpoint
    };


static Checkpoints::MapCheckpoints mapCheckpointsTestnet =
        boost::assign::map_list_of
        ( 12000, uint256S("96c6c3ad38d5f7102d5e14ed730befe4c0865777caac2392ae658dc1b7acc20f"))
;

static const Checkpoints::CCheckpointData dataTestnet = {
        &mapCheckpointsTestnet,
        1338180505,
        16341,
        300
    };
    
static Checkpoints::MapCheckpoints mapCheckpointsRegtest =
        boost::assign::map_list_of
        ( 0, uint256S("50f80e3dea383a355eb15e4be1f122acbc4144bceaa86604555953c5b8a0c9e4"))
        ;
static const Checkpoints::CCheckpointData dataRegtest = {
        &mapCheckpointsRegtest,
        0,
        0,
        0
    };

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.nSubsidyHalvingInterval = 800000;
        consensus.nMajorityEnforceBlockUpgrade = 12960;
        consensus.nMajorityRejectBlockOutdated = 16416;
        consensus.nMajorityWindow = 17280;
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); 
        consensus.nPowTargetTimespan = 2 * 60 * 60; // Lycancoin: 2 hour
        consensus.nPowTargetSpacing = 150; // Lycancoin: 2.5 minute blocks
        consensus.fPowAllowMinDifficultyBlocks = false;
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0xfc;
        pchMessageStart[1] = 0xd9;
        pchMessageStart[2] = 0xb7;
        pchMessageStart[3] = 0xdd;
        vAlertPubKey = ParseHex("04fc9702847840aaf195de8442ebecedf5b095cdbb9bc716bda9110971b28a49e0ead8564ff0db22209e0374782c093bb899692d524e9d6a6956e7c5ecbcd68284");
        nDefaultPort = 58862;
        nMinerThreads = 0;
        nPruneAfterHeight = 100000;

        // Build the genesis block. Note that the output of the genesis coinbase cannot
        // be spent as it did not originally exist in the database.
        //
        // CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
        //   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
        //     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
        //     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
        //   vMerkleTree: 4a5e1e
        const char* pszTimestamp = "Lycancoin Build V.1.0 January 22, 2014";
        CMutableTransaction txNew;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].nValue = 0;
        txNew.vout[0].scriptPubKey = CScript() << 0x0 << OP_CHECKSIG;
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock.SetNull();
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 1;
        genesis.nTime    = 1391485370;
        genesis.nBits    = 0x1e0ffff0;
        genesis.nNonce   = 6678936;

        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x50f80e3dea383a355eb15e4be1f122acbc4144bceaa86604555953c5b8a0c9e4")); //1391485370
        assert(genesis.hashMerkleRoot == uint256S("0x269910b6413f0b424d62db021fed2758ce6761f9b45f5e3a7640ef9dfbe2c218"));

        vSeeds.push_back(CDNSSeedData("lycancoin.org", "seed.lycancoin.org"));
        vSeeds.push_back(CDNSSeedData("lycancoin.org", "seed2.lycancoin.org"));
        vSeeds.push_back(CDNSSeedData("lycancoin.org", "seed3.lycancoin.org"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,48);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));
        
        fRequireRPCPassword = true;
        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = false;
    }

    const Checkpoints::CCheckpointData& Checkpoints() const 
    {
        return data;
    }
};
static CMainParams mainParams;

//
// Testnet (v3)
//

class CTestNetParams : public CMainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nMajorityEnforceBlockUpgrade = 51;
        consensus.nMajorityRejectBlockOutdated = 75;
        consensus.nMajorityWindow = 100;
        consensus.fPowAllowMinDifficultyBlocks = true;
        pchMessageStart[0] = 0x0b;
        pchMessageStart[1] = 0x11;
        pchMessageStart[2] = 0x09;
        pchMessageStart[3] = 0x07;
        vAlertPubKey = ParseHex("04302390343f91cc401d56d68b123028bf52e5fca1939df127f63c6467cdf9c8e2c14b61104cf817d0b780da337893ecc4aaff1309e536162dabbdb45200ca2b0a");
        nDefaultPort = 18333;
        nMinerThreads = 0;
        nPruneAfterHeight = 1000;

        // Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.nTime = 1296688602;
        genesis.nNonce = 414098458;
        consensus.hashGenesisBlock = genesis.GetHash();
//        assert(hashGenesisBlock == uint256S("0x000000000933ea01ad0ee984209779baaec3ced90fa3f408719526f8d77f4943"));

        vFixedSeeds.clear();
        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("alexykot.me", "testnet-seed.alexykot.me"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();
        
        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fRequireRPCPassword = true;
        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;
    }
    const Checkpoints::CCheckpointData& Checkpoints() const 
    {
        return dataTestnet;
    }
};
static CTestNetParams testNetParams;

//
// Regression test
//
class CRegTestParams : public CTestNetParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 150;
        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nMinerThreads = 1;
        genesis.nTime = 1296688602;
        genesis.nBits = 0x207fffff;
        genesis.nNonce = 2;
        consensus.hashGenesisBlock = genesis.GetHash();
        nDefaultPort = 18444;
//        assert(hashGenesisBlock == uint256S("0x0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"));
        nPruneAfterHeight = 1000;
        
        vFixedSeeds.clear(); // Regtest mode doesn't have any fixed seeds.        
        vSeeds.clear();  // Regtest mode doesn't have any DNS seeds.

        fRequireRPCPassword = false;
        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = false;
    }
    const Checkpoints::CCheckpointData& Checkpoints() const 
    {
        return dataRegtest;
    }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = 0;

const CChainParams &Params() {
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams &Params(CBaseChainParams::Network network) {
    switch (network) {
        case CBaseChainParams::MAIN:
            return mainParams;
        case CBaseChainParams::TESTNET:
            return testNetParams;
        case CBaseChainParams::REGTEST:
            return regTestParams;
        default:
            assert(false && "Unimplemented network");
            return mainParams;
    }
}

void SelectParams(CBaseChainParams::Network network) {
    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}

bool SelectParamsFromCommandLine()
{
    CBaseChainParams::Network network = NetworkIdFromCommandLine();
    if (network == CBaseChainParams::MAX_NETWORK_TYPES)
        return false;
    
    SelectParams(network);
    return true;
}