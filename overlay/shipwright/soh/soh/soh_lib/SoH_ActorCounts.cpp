/*
 * SoH_ActorCounts.cpp — OUR overlay file (not upstream Ship of Harkinian).
 *
 * Works around a SoH savestate gap: the per-actor-type load counters
 * (ActorDB Entry.numLoaded) live in the C++ ActorDB, OUTSIDE gSystemHeap, so the
 * savestate never captures them. load_state() overwrites the live actors via a
 * raw memcpy without the normal Actor_FreeOverlay path, so numLoaded leaks upward
 * on every restore and eventually trips `assert(dbEntry->numLoaded < 255)` in
 * Actor_Spawn (z_actor.c) — a hard crash after enough RL resets.
 *
 * Fix: snapshot every numLoaded right after save_state, restore them right after
 * load_state, keeping the counters consistent with the restored heap. This keeps
 * load_state-based resets fast. (RNG and other static state outside gSystemHeap
 * are a separate, non-crashing gap — see the clean-authoring milestone.)
 *
 * Lives under soh/soh/ (globbed for .cpp) and uses C++ (ActorDB is a C++ class).
 */

#ifdef SOH_AS_LIB

#include <vector>
#include "soh/ActorDB.h"

static std::vector<int> g_numLoadedSnapshot;

extern "C" void SoH_SnapshotActorCounts(void) {
    ActorDB* db = ActorDB::Instance;
    if (db == nullptr) {
        return;
    }
    const int n = db->GetEntryCount();
    g_numLoadedSnapshot.assign((size_t)n, 0);
    for (int i = 0; i < n; ++i) {
        g_numLoadedSnapshot[(size_t)i] = db->RetrieveEntry(i).entry.numLoaded;
    }
}

extern "C" void SoH_RestoreActorCounts(void) {
    ActorDB* db = ActorDB::Instance;
    if (db == nullptr || g_numLoadedSnapshot.empty()) {
        return;
    }
    const int n = db->GetEntryCount();
    const int m = (int)g_numLoadedSnapshot.size();
    for (int i = 0; i < n && i < m; ++i) {
        db->RetrieveEntry(i).entry.numLoaded = g_numLoadedSnapshot[(size_t)i];
    }
}

#endif /* SOH_AS_LIB */
