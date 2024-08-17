#pragma once

#include <cassert>
#include <cstdint>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#ifdef _WIN32
# define NOMINMAX
# include <Windows.h>
#endif

#include "randomx.h"

#define NOW std::chrono::system_clock::now()
#define SECS(x) static_cast<std::chrono::duration<double>>(x).count()

typedef  uint8_t  u8;
typedef uint16_t u16;
typedef  int32_t s32;
typedef uint32_t u32;
typedef uint64_t u64;

namespace wxpower
{
    template <typename K>
    inline void sortedAddIfNotIn(std::vector<K>& cont, const K& key)
    {
        typename std::vector<K>::const_iterator it =
            std::lower_bound(cont.cbegin(), cont.cend(), key);

        if ((it == cont.cend()) || (*it != key))
        {
            assert((it == cont.cend()) || (key < *it));
            cont.insert(it, key);
        }
    }

    bool strToDbl(const std::string& in, double& out);
    bool strToU32(const std::string& in, u32& out);

    char binToHex(u8 bin);

    /* Simple storage for 256-bit hash outputs. */
    class Bigint
    {
    public:
        Bigint();
        u8* getBytes();
        const u8* getBytes() const;
        std::string toString() const;

    private:
        u8 buf[32];
    };

    /* Minimal, custom base 64 class for seed printing. */
    class Base64
    {
    public:
        Base64();
        Base64(u64 val);
        Base64(u32 val);

        void incr();
        std::string toString() const;

        static const std::vector<char> mapping;

    private:
        static const u32 maxSize = 64;
        u8 buf[maxSize];
        u32 len = 1;
    };

    /* Class to access SHA256. On Windows, uses built-in 'Bcrypt.lib' funcs to
    * minimise dependencies. If you're on Linux then it requires 'openssl-dev',
    * but you don't need me to tell you that :). */
    class Sha256
    {
    public:
        Sha256();
        virtual ~Sha256();

        Bigint doHash(const char* data, const u32 len);

        bool isValid() const;

    private:
        bool valid = true;

#ifdef _WIN32
        BCRYPT_HASH_HANDLE hash = nullptr;
        PBYTE hashObj = nullptr;
        DWORD hashObjSize = 0;
#endif
    };

    bool setThreadAffinity(std::thread& thread, u32 core);

    struct HashResult
    {
        std::string proof;
        Bigint hash;
        u32 diff = 0;
    };

    class PowerBase
    {
    public:
        virtual ~PowerBase() = default;

        /* Only verify is a function to implement as it is the only
           functionality which is uncertain at runtime. Proving is
           always done with a known version, so it is omitted here
           and implemented specifically in each case. */
        virtual bool verifyMessage(
            const std::string& proof, bool useLargePages,
            std::optional<HashResult>& res, std::string& prettyMetaData,
            std::string& error) = 0;

        virtual u32 getVersion() = 0;

        static PowerBase* createInstance(u32 version);

        static constexpr u32 latestVersion = 0;
    };

    class PowerV0 : public PowerBase
    {
    public:
        struct ProofContent
        {
            std::string body;
            u32 version = 0;
            std::string userId;
            std::string context;
        };

        virtual ~PowerV0() = default;

        /* Returns true if a message was parsed with this API version
           correctly. Note that the actual verification may still fail,
           i.e. if large pages were requested but not supported. */
        virtual bool verifyMessage(
            const std::string& proof, bool useLargePages,
            std::optional<HashResult>& res, std::string& prettyMetaData,
            std::string& error) override;

        u32 getVersion() override
        {
            return 0;
        }

        /* Calculate "left zeroes count" difficulty. */
        static u32 calcLZCDiff(const Bigint& bigint);

        /* Concatenates info into the metadata string. (This is hashed with
           SHA256 and used as the RX K input.) */
        static std::string contentToMetaData(
            const PowerV0::ProofContent& content);

        static s32 utf8CharsToSkip(const std::string& str, s32 pos);

        /* Removes non-printable characters from the input string. */
        static void trimBody(std::string& body);

        static const std::string magicSequence;
        static const std::vector<char> skippable1;
        static const std::vector<std::string> skippable2;
        static const std::vector<std::string> skippable3;


    protected:
        /* Extracts information from a proof for verification purposes. */
        virtual bool msgToContent(ProofContent& proof, const std::string& msg);
    };

    using NowTime = std::chrono::system_clock::time_point;
    using AbsTime = std::chrono::time_point<
        std::chrono::system_clock, std::chrono::duration<
        double, std::chrono::system_clock::period>>;

    class ProveV0Manager
    {
    public:
        enum class State
        {
            rxIniting,
            rxFailed,
            rxCancelled,
            hashing,
            hashCancelled,
            finished
        };

        static bool isStoppedState(const State s);

        /* Members of a v0 prove state which may only be accessed/modified
           when holding the `masterMutex`. */
        struct MasterGuarded
        {
            // Number of threads still hashing
            u32 threadsRunning = UINT32_MAX;

            // Is it safe for the GUI to examine the thread-specific data?
            bool threadsActive = false;

            // If true, the object is safe to delete
            bool masterFinished = false;

            std::string errorStr;
            std::vector<std::string> warnings;

            std::optional<double> rxTime;
            std::optional<NowTime> hashStartTime;
            std::optional<NowTime> hashStopTime;

            /* `bestResults` only updated when threads quit. Read `bestDiff` to
               see progress during runtime. */
            std::vector<HashResult> bestResults;
        };

        struct ThreadGuarded
        {
            std::vector<std::atomic<u32>*> bestDiff;
            std::vector<std::atomic<u64>*> hashes;
        };

        struct ThreadGuardedRet
        {
            std::vector<u32> bestDiff;
            std::vector<u64> hashes;
        };

        ProveV0Manager(
            const PowerV0::ProofContent& content_,
            const std::vector<u32>& initCores_,
            const std::vector<u32>& hashCores_,
            bool useLargePages_,
            const std::optional<u32>& diff_,
            const std::optional<double>& timeLimit_);

        ~ProveV0Manager();

        void cancel();
        bool isCancelled() const;

        MasterGuarded getMasterGuardedData();
        ThreadGuardedRet getThreadGuardedData() const;
        State getState() const;

        const PowerV0::ProofContent content;
        const std::vector<u32> initCores;
        const std::vector<u32> hashCores;
        const bool useLargePages;
        const std::optional<u32> diff;
        const std::optional<double> timeLimit;
        const u32 hashThreadCount;

    private:
        static void threadEntry(ProveV0Manager* state);
        static void hashThreadEntry(ProveV0Manager* mgr, u32 tid, randomx_vm* const vm);

        std::optional<std::thread> master;
        std::mutex masterMutex;
        std::condition_variable masterCond;

        /* Should initing/hashing continue? */
        std::atomic<bool> running;

        /* Has the GUI requested to cancel? */
        std::atomic<bool> cancelled;

        std::vector<std::thread> hashThreads;

        // Members that are guarded by `masterMutex`
        MasterGuarded masterGuarded;

        // Members that are thread-specific
        ThreadGuarded threadGuarded;

        std::atomic<State> state;
    };

    class RxManager
    {
    public:
        // Does not throw on cancellation
        class Exception
        {
        public:
            Exception(const std::string& what_);

            const std::string& getWhat() const noexcept;

        private:
            std::string what;
        };

        // Proving
        RxManager(
            bool useLargePages_, const Bigint& K_,
            const std::vector<u32>& initCores_, u32 hashCores_,
            const std::atomic<bool>& cancelled_);

        // Verification
        RxManager(bool useLargePages_, const Bigint& K_);

        virtual ~RxManager();

        void init();

        std::vector<std::string> warnings;
        std::optional<double> initTime;

        randomx_vm* getVM(u32 tid);

    private:
        static void rxInitDatasetWrapper(
            u32 tid, randomx_dataset* dataset, randomx_cache* cache, unsigned long startItem,
            unsigned long itemCount, const std::atomic<bool>& cancelled);

        const bool proveMode;
        const bool useLargePages;
        const Bigint K;
        const std::vector<u32> initCores;
        const u32 hashCores;

        randomx_flags flags;
        randomx_cache* cache = nullptr;
        randomx_dataset* dataset = nullptr;
        std::vector<randomx_vm*> vms;
    };
}
