//
// Created by 黄鸿 on 2023/8/11.
//

#ifndef RE2_CLION_DFA_H
#define RE2_CLION_DFA_H

#endif //RE2_CLION_DFA_H

#include <ctime>
#include <ratio>
#include <chrono>
#include "prog.h"
#include "util/mutex.h"
#include "util/mix.h"
using namespace std;
namespace re2 {
    class DFA {
    public:
        DFA(Prog *prog, Prog::MatchKind kind, int64_t max_mem);

        ~DFA();

        bool ok() const { return !init_failed_; }

        Prog::MatchKind kind() { return kind_; }

        enum {
            Bit1 = 7,
            Bitx = 6,
            Bit2 = 5,
            Bit3 = 4,
            Bit4 = 3,
            Bit5 = 2,

            T1 = ((1 << (Bit1 + 1)) - 1) ^ 0xFF,    /* 0000 0000 */
            Tx = ((1 << (Bitx + 1)) - 1) ^ 0xFF,    /* 1000 0000 */
            T2 = ((1 << (Bit2 + 1)) - 1) ^ 0xFF,    /* 1100 0000 */
            T3 = ((1 << (Bit3 + 1)) - 1) ^ 0xFF,    /* 1110 0000 */
            T4 = ((1 << (Bit4 + 1)) - 1) ^ 0xFF,    /* 1111 0000 */
            T5 = ((1 << (Bit5 + 1)) - 1) ^ 0xFF    /* 1111 1000 */
        };

        // Searches for the regular expression in text, which is considered
        // as a subsection of context for the purposes of interpreting flags
        // like ^ and $ and \A and \z.
        // Returns whether a match was found.
        // If a match is found, sets *ep to the end point of the best match in text.
        // If "anchored", the match must begin at the start of text.
        // If "want_earliest_match", the match that ends first is used, not
        //   necessarily the best one.
        // If "run_forward" is true, the DFA runs from text.begin() to text.end().
        //   If it is false, the DFA runs from text.end() to text.begin(),
        //   returning the leftmost end of the match instead of the rightmost one.
        // If the DFA cannot complete the search (for example, if it is out of
        //   memory), it sets *failed and returns false.
        bool Search(const StringPiece &text, const StringPiece &context,
                    bool anchored, bool want_earliest_match, bool run_forward,
                    bool *failed, const char **ep, SparseSet *matches);

        // Builds out all states for the entire DFA.
        // If cb is not empty, it receives one callback per state built.
        // Returns the number of states built.
        // FOR TESTING OR EXPERIMENTAL PURPOSES ONLY.
        int BuildAllStates(const Prog::DFAStateCallback &cb);

        class RWLocker {
        public:
            explicit RWLocker(Mutex* mu);
            ~RWLocker();

            // If the lock is only held for reading right now,
            // drop the read lock and re-acquire for writing.
            // Subsequent calls to LockForWriting are no-ops.
            // Notice that the lock is *released* temporarily.
            void LockForWriting();

        private:
            Mutex* mu_;
            bool writing_;

            RWLocker(const RWLocker&) = delete;
            RWLocker& operator=(const RWLocker&) = delete;
        };

        class StateSaver;

        class Workq;

        int Hamilton_Deep_Search(int length, int restart_times, std::string regex, int regex_id, string ReDoS_file);

        int Hamilton_Deep_Random_Search(int length);

        int Hamilton_Deep_nStepForward_Search(int length, int ForwardLength);

        int Hamilton_Deep_MoreUnbgous_Search(int length);

        int Hamilton_Deep_Mul_Search();


        // Computes min and max for matching strings.  Won't return strings
        // bigger than maxlen.
        bool PossibleMatchRange(std::string *min, std::string *max, int maxlen);

        // These data structures are logically private, but C++ makes it too
        // difficult to mark them as such.


        // A single DFA state.  The DFA is represented as a graph of these
        // States, linked by the next_ pointers.  If in state s and reading
        // byte c, the next state should be s->next_[c].
        struct State {
            inline bool IsMatch() const { return (flag_ & kFlagMatch) != 0; }

            int *inst_;         // Instruction pointers in the state.
            int ninst_;         // # of inst_ pointers.
            uint32_t flag_;     // Empty string bitfield flags in effect on the way
            // into this state, along with kFlagMatch if this
            // is a matching state.
            /*std::set<State*> child;
            State* parent;
            int deep;*/
// Work around the bug affecting flexible array members in GCC 6.x (for x >= 1).
// (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70932)
#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ == 6 && __GNUC_MINOR__ >= 1
            std::atomic<State*> next_[0];   // Outgoing arrows from State,
#else
            std::atomic<State *> next_[];    // Outgoing arrows from State,
#endif
            // one per input byte class
        };

        std::multimap<int, int> Char_Weight(int length, State *begin);
        int BFS_DFA_Cover_Random(int length, std::string regex, int regex_id, std::string ReDoS_file);
        int BFS_DFA_Cover(int length, std::string regex, int regex_id, std::string ReDoS_file);
        int BFS_DFA_Hy(int length, std::string regex, int regex_id, std::string ReDoS_file);
        int Mul_online_match(int length, std::string regex, int regex_id);
        std::pair<int, bool>
        HamiltonDeep(std::set<State *> S_m, std::deque<char> Rune_q, State *T, std::vector<int> input, int n);

        int OutPathNum(State *ns, std::vector<int> input, std::set<State *> StateSet, int Deep_Length);

        void RuneToStringFile(std::deque<int> Rune_q);

        float count_InstIDNum_InState(State* s, int upper_bound);
        //数据结构声明；并且对key的查找操作进行定义，这样才能进行find等操作



        //void IncreaseDeep(State *s, int n, std::unordered_map<State *, std::string> str_set, std::string string);

        //bool FindIfFather(State *s, State *ns);

        std::deque<unsigned int>
        FindOnePath(State *s, std::set<State *> Sd, std::deque<unsigned int>, std::vector<int> input);

        enum {
            kByteEndText = 256,         // imaginary byte at end of text
            kFlagEmptyMask = 0xFF,      // State.flag_: bits holding kEmptyXXX flags
            kFlagMatch = 0x0100,        // State.flag_: this is a matching state
            kFlagLastWord = 0x0200,     // State.flag_: last byte was a word char
            kFlagNeedShift = 16,        // needed kEmpty bits are or'ed in shifted left
        };

        struct StateHash {
            size_t operator()(const State *a) const {
                DCHECK(a != NULL);
                HashMix mix(a->flag_);
                for (int i = 0; i < a->ninst_; i++)
                    mix.Mix(a->inst_[i]);
                mix.Mix(0);
                return mix.get();
            }
        };

        struct StateEqual {
            bool operator()(const State *a, const State *b) const {
                DCHECK(a != NULL);
                DCHECK(b != NULL);
                if (a == b)
                    return true;
                if (a->flag_ != b->flag_)
                    return false;
                if (a->ninst_ != b->ninst_)
                    return false;
                for (int i = 0; i < a->ninst_; i++)
                    if (a->inst_[i] != b->inst_[i])
                        return false;
                return true;
            }
        };

        typedef std::unordered_set<State *, StateHash, StateEqual> StateSet;

    private:
        // Make it easier to swap in a scalable reader-writer mutex.
        using CacheMutex = Mutex;

        enum {
            // Indices into start_ for unanchored searches.
            // Add kStartAnchored for anchored searches.
            kStartBeginText = 0,          // text at beginning of context
            kStartBeginLine = 2,          // text at beginning of line
            kStartAfterWordChar = 4,      // text follows a word character
            kStartAfterNonWordChar = 6,   // text follows non-word character
            kMaxStart = 8,

            kStartAnchored = 1,
        };

        // Resets the DFA State cache, flushing all saved State* information.
        // Releases and reacquires cache_mutex_ via cache_lock, so any
        // State* existing before the call are not valid after the call.
        // Use a StateSaver to preserve important states across the call.
        // cache_mutex_.r <= L < mutex_
        // After: cache_mutex_.w <= L < mutex_
        void ResetCache(RWLocker *cache_lock);

        // Looks up and returns the State corresponding to a Workq.
        // L >= mutex_
        State *WorkqToCachedState(Workq *q, Workq *mq, uint32_t flag);

        // Looks up and returns a State matching the inst, ninst, and flag.
        // L >= mutex_
        State *CachedState(int *inst, int ninst, uint32_t flag);

        // Clear the cache entirely.
        // Must hold cache_mutex_.w or be in destructor.
        void ClearCache();

        // Converts a State into a Workq: the opposite of WorkqToCachedState.
        // L >= mutex_
        void StateToWorkq(State *s, Workq *q);

        // Runs a State on a given byte, returning the next state.
        State *RunStateOnByteUnlocked(State *, int);  // cache_mutex_.r <= L < mutex_
        State *RunStateOnByte(State *, int);          // L >= mutex_

        // Runs a Workq on a given byte followed by a set of empty-string flags,
        // producing a new Workq in nq.  If a match instruction is encountered,
        // sets *ismatch to true.
        // L >= mutex_
        void RunWorkqOnByte(Workq *q, Workq *nq,
                            int c, uint32_t flag, bool *ismatch);

        // Runs a Workq on a set of empty-string flags, producing a new Workq in nq.
        // L >= mutex_
        void RunWorkqOnEmptyString(Workq *q, Workq *nq, uint32_t flag);

        // Adds the instruction id to the Workq, following empty arrows
        // according to flag.
        // L >= mutex_
        void AddToQueue(Workq *q, int id, uint32_t flag);

        // For debugging, returns a text representation of State.
        static std::string DumpState(State *state);

        // For debugging, returns a text representation of a Workq.
        static std::string DumpWorkq(Workq *q);

        // Search parameters
        struct SearchParams {
            SearchParams(const StringPiece &text, const StringPiece &context,
                         RWLocker *cache_lock)
                    : text(text),
                      context(context),
                      anchored(false),
                      can_prefix_accel(false),
                      want_earliest_match(false),
                      run_forward(false),
                      start(NULL),
                      cache_lock(cache_lock),
                      failed(false),
                      ep(NULL),
                      matches(NULL) {}

            StringPiece text;
            StringPiece context;
            bool anchored;
            bool can_prefix_accel;
            bool want_earliest_match;
            bool run_forward;
            State *start;
            RWLocker *cache_lock;
            bool failed;     // "out" parameter: whether search gave up
            const char *ep;  // "out" parameter: end pointer for match
            SparseSet *matches;

        private:
            SearchParams(const SearchParams &) = delete;

            SearchParams &operator=(const SearchParams &) = delete;
        };

        // Before each search, the parameters to Search are analyzed by
        // AnalyzeSearch to determine the state in which to start.
        struct StartInfo {
            StartInfo() : start(NULL) {}

            std::atomic<State *> start;
        };

        // Fills in params->start and params->can_prefix_accel using
        // the other search parameters.  Returns true on success,
        // false on failure.
        // cache_mutex_.r <= L < mutex_
        bool AnalyzeSearch(SearchParams *params);

        bool AnalyzeSearchHelper(SearchParams *params, StartInfo *info,
                                 uint32_t flags);

        // The generic search loop, inlined to create specialized versions.
        // cache_mutex_.r <= L < mutex_
        // Might unlock and relock cache_mutex_ via params->cache_lock.
        template<bool can_prefix_accel,
                bool want_earliest_match,
                bool run_forward>
        inline bool InlinedSearchLoop(SearchParams *params);

        // The specialized versions of InlinedSearchLoop.  The three letters
        // at the ends of the name denote the true/false values used as the
        // last three parameters of InlinedSearchLoop.
        // cache_mutex_.r <= L < mutex_
        // Might unlock and relock cache_mutex_ via params->cache_lock.
        bool SearchFFF(SearchParams *params);

        bool SearchFFT(SearchParams *params);

        bool SearchFTF(SearchParams *params);

        bool SearchFTT(SearchParams *params);

        bool SearchTFF(SearchParams *params);

        bool SearchTFT(SearchParams *params);

        bool SearchTTF(SearchParams *params);

        bool SearchTTT(SearchParams *params);

        // The main search loop: calls an appropriate specialized version of
        // InlinedSearchLoop.
        // cache_mutex_.r <= L < mutex_
        // Might unlock and relock cache_mutex_ via params->cache_lock.
        bool FastSearchLoop(SearchParams *params);


        // Looks up bytes in bytemap_ but handles case c == kByteEndText too.
        int ByteMap(int c) {
            if (c == kByteEndText)
                return prog_->bytemap_range();
            return prog_->bytemap()[c];
        }

        // Constant after initialization.
        Prog *prog_;              // The regular expression program to run.
        Prog::MatchKind kind_;    // The kind of DFA.
        bool init_failed_;        // initialization failed (out of memory)

        Mutex mutex_;  // mutex_ >= cache_mutex_.r

        // Scratch areas, protected by mutex_.
        Workq *q0_;             // Two pre-allocated work queues.
        Workq *q1_;
        PODArray<int> stack_;   // Pre-allocated stack for AddToQueue

        // State* cache.  Many threads use and add to the cache simultaneously,
        // holding cache_mutex_ for reading and mutex_ (above) when adding.
        // If the cache fills and needs to be discarded, the discarding is done
        // while holding cache_mutex_ for writing, to avoid interrupting other
        // readers.  Any State* pointers are only valid while cache_mutex_
        // is held.
        CacheMutex cache_mutex_;
        int64_t mem_budget_;     // Total memory budget for all States.
        int64_t state_budget_;   // Amount of memory remaining for new States.
        StateSet state_cache_;   // All States computed so far.
        StartInfo start_[kMaxStart];

        DFA(const DFA &) = delete;

        DFA &operator=(const DFA &) = delete;

    };
}