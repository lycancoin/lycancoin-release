// Copyright (c) 2012 The Bitcoin developers
// Copyright (c) 2012 Litecoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_VERSION_H
#define BITCOIN_VERSION_H
 
//
// network protocol versioning
//

static const int PROTOCOL_VERSION = 60004;

// intial proto version, to be increased after version/verack negotiation
static const int INIT_PROTO_VERSION = 60000;

// In this version, 'getheaders' was introduced.
static const int GETHEADERS_VERSION = 60004;

// disconnect from peers older than this proto version
static const int MIN_PEER_PROTO_VERSION = GETHEADERS_VERSION;

// nTime field added to CAddress, starting with this version;
// if possible, avoid requesting addresses nodes older than this
static const int CADDR_TIME_VERSION = 60004;

// BIP 0031, pong message, is enabled for all versions AFTER this one
static const int BIP0031_VERSION = 60000;

// "mempool" command, enhanced "getdata" behavior starts with this version:
static const int MEMPOOL_GD_VERSION = 60002;

#endif // BITCOIN_VERSION_H