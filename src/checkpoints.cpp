// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2011-2012 Litecoin Developers
// Copyright (c) 2014 Lycancoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

#include "checkpoints.h"

#include "main.h"
#include "uint256.h"

namespace Checkpoints
{
    typedef std::map<int, uint256> MapCheckpoints;

    //
    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    //
    static MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of // Checkpoint 0 hash == Genesis block hash.
        (         0, uint256("0x50f80e3dea383a355eb15e4be1f122acbc4144bceaa86604555953c5b8a0c9e4"))
        (	  12000, uint256("0x96c6c3ad38d5f7102d5e14ed730befe4c0865777caac2392ae658dc1b7acc20f"))
        (	  17250, uint256("0xf4f5d8b8928fe9f3f434b27096eebee85874c4d855a8e6c732c6c34765e32a13"))
        (	  24250, uint256("0x39170f93af0eee880dbdb25f583feb3f48fe4146bcb7d2fe100bc8708407b680"))
        (	  72000, uint256("0xc86a831fa0aa3511d746db05427d01fee1ff18192db12817d37d561f9bbfe585"))
        (    140000, uint256("0xf9d5d85ca836627ba4d4463427182a11e25502b505c91124d587e1284b024d79"))
        (   1380000, uint256("0x6a3eb7216bf241846f8f370d391673e422ab3352821284fb4b75b8cf0d67071d"))
        ;


    static MapCheckpoints mapCheckpointsTestnet =
        boost::assign::map_list_of
        ( 546, uint256("0x50f80e3dea383a355eb15e4be1f122acbc4144bceaa86604555953c5b8a0c9e4"))
;

    bool CheckBlock(int nHeight, const uint256& hash)
    {
    	  if (!GetBoolArg("-checkpoints", true))
            return true;

        MapCheckpoints& checkpoints = (fTestNet ? mapCheckpointsTestnet : mapCheckpoints);

        MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        if (i == checkpoints.end()) return true;
        return hash == i->second;
}

    int GetTotalBlocksEstimate()
    {
    	  if (!GetBoolArg("-checkpoints", true))
            return 0;
            
        MapCheckpoints& checkpoints = (fTestNet ? mapCheckpointsTestnet : mapCheckpoints);

        return checkpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        if (!GetBoolArg("-checkpoints", true))
            return NULL;

        MapCheckpoints& checkpoints = (fTestNet ? mapCheckpointsTestnet : mapCheckpoints);

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, checkpoints)
        {
            const uint256& hash = i.second;
            std::map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }
}