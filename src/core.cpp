#include <cctype>
#include <cerrno>
#include <cinttypes>
#include <cmath>
#include <cstring>

#include <algorithm>
#include <array>
#include <exception>
#include <iostream>
#include <sstream>
#include <string_view>
#include <type_traits>

#ifndef _WIN32
# include <sys/random.h>
# include <sched.h>
# include <pthread.h>
# include <openssl/evp.h>
#endif

#include "power.hpp"

#include "configuration.h"

static_assert(CHAR_BIT == 8, "CHAR_BIT != 8");
static_assert(RANDOMX_HASH_SIZE == 32, "RANDOMX_HASH_SIZE != 32");

// Sanity check that RX was configured correctly.
// Any items not listed here are their default values as in RX v1.2.1.
static_assert(RANDOMX_ARGON_ITERATIONS == 2, "Incorrect RX build configuration");
static_assert(std::string_view(RANDOMX_ARGON_SALT) == "wxPoWer\x03", "Incorrect RX build configuration");
static_assert(RANDOMX_CACHE_ACCESSES == 10, "Incorrect RX build configuration");
static_assert(RANDOMX_DATASET_EXTRA_SIZE == 33554304, "Incorrect RX build configuration");
static_assert(RANDOMX_PROGRAM_SIZE == 192, "Incorrect RX build configuration");

namespace wxpower
{
    bool strToDbl(const std::string& in, double& out)
    {
        bool ret = false;
        errno = 0;

        const double temp = std::strtod(in.c_str(), nullptr);

        if ((temp != HUGE_VAL) && (temp != -HUGE_VAL) &&
            (errno != ERANGE) && (errno != EINVAL))
        {
            out = temp;
            ret = true;
        }

        return ret;
    }

    bool strToU32(const std::string& in, u32& out)
    {
        bool ret = false;
        const unsigned long temp = std::strtoul(in.c_str(), nullptr, 10);

        if (temp != ULONG_MAX)
        {
            out = static_cast<u32>(temp);
            ret = true;
        }

        return ret;
    }

    bool setThreadAffinity(std::thread& thread, u32 core)
    {
        bool ret = true;

#ifdef _WIN32
        const ULONG_PTR newMask =
            static_cast<ULONG_PTR>(1) << core;

        const DWORD_PTR res = SetThreadAffinityMask(
            thread.native_handle(), newMask);

        if (res == 0)
            ret = false;
#else
        using native_handle_type = std::thread::native_handle_type;

        static_assert(
            std::is_same<native_handle_type, pthread_t>::value,
            "Expected pthread_t handle");

        cpu_set_t cpuSet;

        CPU_ZERO(&cpuSet);
        CPU_SET(core, &cpuSet);

        const int res = pthread_setaffinity_np(
            thread.native_handle(), sizeof(cpuSet), &cpuSet);

        if (res != 0)
            ret = false;
#endif

        return ret;
    }

    char binToHex(u8 bin)
    {
        assert(bin <= 0x0f);

        if (bin > UINT8_C(9))
            return (char)(bin + UINT8_C(0x57));
        else
            return (char)(bin + UINT8_C(0x30));
    }

    Bigint::Bigint()
    {
        memset(buf, 0xff, 32);
    }

    u8* Bigint::getBytes()
    {
        return buf;
    }

    const u8* Bigint::getBytes() const
    {
        return buf;
    }

    std::string Bigint::toString() const
    {
        char charBuf[32 * 2 + 1];

        for (u32 i = 0; i < 32; i++)
        {
            // Upper nibble
            u8 un = (0xf0 & buf[i]) >> 4;

            // Lower nibble
            u8 ln = 0x0f & buf[i];

            charBuf[i * 2] = binToHex(un);
            charBuf[i * 2 + 1] = binToHex(ln);
        }

        charBuf[32 * 2] = 0;

        std::string ret = charBuf;

        return ret;
    }

    Base64::Base64()
        : buf{ 0 }
    {
    }

    Base64::Base64(u64 val)
        : buf{ 0 }
    {
        for (u32 n = 0; n < 64; n += 6)
            buf[n / 6] = static_cast<u8>((val >> n) & 0x3f);

        for (u32 n = 64 / 6; n > 0; n--)
            if (buf[n] > 0)
            {
                len = n + 1;
                break;
            }
    }

    Base64::Base64(u32 val)
        : Base64(static_cast<u64>(val))
    {}

    void Base64::incr()
    {
        u32 i = 0;

        for (; i < maxSize; i++)
        {
            buf[i]++;

            if (buf[i] <= 0x3f)
                break;
            else
                buf[i] = 0;
        }

        if (i >= len)
            len = i + 1;
    }

    std::string Base64::toString() const
    {
        std::string ret;

        for (u32 i = 0; i < len; i++)
        {
            assert(buf[i] <= 0x3f);
            ret += mapping.at(buf[i]);
        }

        return ret;
    }

    const std::vector<char> Base64::mapping = {
    /*  0 */ 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    /*  8 */ 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    /* 16 */ 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    /* 24 */ 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    /* 32 */ 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    /* 40 */ 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    /* 48 */ 'w', 'x', 'y', 'z', '0', '1', '2', '3',
    /* 56 */ '4', '5', '6', '7', '8', '9', '-', '_'
    };

    Sha256::Sha256()
    {
#ifdef _WIN32
        BCRYPT_ALG_HANDLE alg = nullptr;
        DWORD hashLen = 0;
        DWORD data = 0;

        NTSTATUS res = BCryptOpenAlgorithmProvider(
            &alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);

        if (res < 0)
        {
            std::cerr << "BCryptOpenAlgorithmProvider failed\n";
            valid = false;
        }

        if (valid)
        {
            res = BCryptGetProperty(
                alg, BCRYPT_OBJECT_LENGTH,
                reinterpret_cast<PBYTE>(&hashObjSize), sizeof(DWORD),
                &data, 0);

            if (res < 0)
            {
                std::cerr << "BCryptGetProperty failed\n";
                valid = false;
            }
        }

        hashObj = new BYTE[hashObjSize];

        if (valid)
        {
            res = BCryptCreateHash(
                alg, &hash, hashObj, hashObjSize, nullptr, 0,
                BCRYPT_HASH_REUSABLE_FLAG);

            if (res < 0)
            {
                std::cerr << "BCryptCreateHash failed\n";
                valid = false;
            }
        }

        if (valid)
        {
            res = BCryptGetProperty(alg, BCRYPT_HASH_LENGTH,
                reinterpret_cast<PBYTE>(&hashLen), sizeof(DWORD),
                &data, 0);

            if (res < 0)
            {
                std::cerr << "BCryptGetProperty failed\n";
                valid = false;
            }
            else if (hashLen != 32)
            {
                std::cerr << "Unexpected hash length\n";
                valid = false;
            }
        }
#endif
    }

    Sha256::~Sha256()
    {
#ifdef _WIN32
        NTSTATUS res = BCryptDestroyHash(hash);

        if (res < 0)
            std::cerr << "BCryptDestroyHash failed\n";

        delete[] hashObj;
#endif
    }

    Bigint Sha256::doHash(const char* data, const u32 len)
    {
        Bigint ret;

#ifdef _WIN32
        NTSTATUS res;

        if (valid)
        {
            res = BCryptHashData(hash, const_cast<PUCHAR>(
                reinterpret_cast<const u8*>(data)), len, 0);

            if (res < 0)
            {
                std::cerr << "BCryptHashData failed\n";
                valid = false;
            }
        }

        if (valid)
        {
            res = BCryptFinishHash(hash, ret.getBytes(), 32, 0);

            if (res < 0)
            {
                std::cerr << "BCryptFinishHash failed\n";
                valid = false;
            }
        }
#else
        size_t mdlen;

        int res = EVP_Q_digest(
            nullptr, "sha256", nullptr, data, len, ret.getBytes(), &mdlen);

        assert(mdlen == 32);
        assert(res == 1);
        valid = (mdlen == 32) && (res == 1);
#endif

        return ret;
    }

    bool Sha256::isValid() const
    {
        return valid;
    }


    PowerBase* PowerBase::createInstance(u32 version)
    {
        if (version == 0)
            return new PowerV0;
        else
            throw std::runtime_error("Unknown version");
    }

    void ProveV0Manager::hashThreadEntry(ProveV0Manager* mgr, u32 tid, randomx_vm* const vm)
    {
        HashResult& finalBestResult = mgr->masterGuarded.bestResults.at(tid);

        HashResult bestResult;
        u64 localHashes = 0;

        std::atomic<u32>* bestDiff = mgr->threadGuarded.bestDiff.at(tid);
        std::atomic<u64>* hashes = mgr->threadGuarded.hashes.at(tid);

        const Base64 threadSeed(tid);
        const std::string metaData = PowerV0::contentToMetaData(mgr->content);
        const std::string msgWithThreadSeed =
            mgr->content.body + '|' + threadSeed.toString() + '|';

        Bigint thisResult;
        Base64 ctrSeed;
        u32 minDiff = 256;

        if (mgr->diff.has_value())
            minDiff = mgr->diff.value();

        while (true)
        {
            if ((localHashes & 0xf) == (tid & 0xf))
            {
                if (!mgr->running.load())
                    break;

                hashes->store(localHashes);
            }

            const std::string H = msgWithThreadSeed + ctrSeed.toString();

            randomx_calculate_hash(vm, H.c_str(), H.size(), thisResult.getBytes());
            const u32 thisDiff = PowerV0::calcLZCDiff(thisResult);

            if (thisDiff > bestResult.diff)
            {
                bestResult.proof = H + '|' + metaData;
                bestResult.hash = thisResult;
                bestResult.diff = thisDiff;
                bestDiff->store(thisDiff);

                if (bestResult.diff >= minDiff)
                {
                    mgr->running.store(false);
                    break;
                }
            }

            ctrSeed.incr();
            localHashes++;
        }

        // Mutex scope
        {
            std::lock_guard<std::mutex> lock(mgr->masterMutex);
            finalBestResult = bestResult;
            hashes->store(localHashes);
            mgr->masterGuarded.threadsRunning--;
            mgr->masterCond.notify_one();
        }
    }

    bool ProveV0Manager::isStoppedState(const ProveV0Manager::State s)
    {
        return (s == State::rxFailed) ||
            (s == State::rxCancelled) ||
            (s == State::hashCancelled) ||
            (s == State::finished);
    }

    ProveV0Manager::ProveV0Manager(
        const PowerV0::ProofContent& content_,
        const std::vector<u32>& initCores_,
        const std::vector<u32>& hashCores_,
        bool useLargePages_,
        const std::optional<u32>& diff_,
        const std::optional<double>& timeLimit_)
        :
        content(content_), initCores(initCores_), hashCores(hashCores_),
        useLargePages(useLargePages_), diff(diff_), timeLimit(timeLimit_),
        hashThreadCount(static_cast<u32>(hashCores.size())),
        running(true), cancelled(false), state(State::rxIniting)
    {
        masterGuarded.threadsRunning = static_cast<u32>(hashCores.size());

        for (size_t i = 0; i < hashCores.size(); i++)
        {
            threadGuarded.bestDiff.push_back(new std::atomic<u32>(0));
            threadGuarded.hashes.push_back(new std::atomic<u64>(0));
        }

        master.emplace(threadEntry, this);
    }

    ProveV0Manager::~ProveV0Manager()
    {
        master->join();

        for (size_t i = 0; i < hashCores.size(); i++)
        {
            delete threadGuarded.hashes.at(i);
            delete threadGuarded.bestDiff.at(i);
        }
    }

    void ProveV0Manager::cancel()
    {
        cancelled.store(true);
        running.store(false);
    }

    bool ProveV0Manager::isCancelled() const
    {
        return cancelled.load();
    }

    ProveV0Manager::MasterGuarded ProveV0Manager::getMasterGuardedData()
    {
        std::lock_guard<std::mutex> lock(masterMutex);
        return masterGuarded;
    }

    ProveV0Manager::ThreadGuardedRet ProveV0Manager::getThreadGuardedData() const
    {
        ProveV0Manager::ThreadGuardedRet ret;

        assert(threadGuarded.bestDiff.size() == threadGuarded.hashes.size());

        for (size_t i = 0; i < threadGuarded.bestDiff.size(); i++)
        {
            ret.bestDiff.push_back(threadGuarded.bestDiff.at(i)->load());
            ret.hashes.push_back(threadGuarded.hashes.at(i)->load());
        }

        return ret;
    }

    ProveV0Manager::State ProveV0Manager::getState() const
    {
#ifndef NDEBUG
        if (masterGuarded.threadsActive)
        {
            assert(masterGuarded.rxTime.has_value());
        }
        else
        {
            assert(!masterGuarded.hashStartTime.has_value());
        }

        if (masterGuarded.hashStopTime.has_value())
        {
            assert(masterGuarded.hashStartTime.has_value());
        }
#endif

        return state.load();
    }

    void ProveV0Manager::threadEntry(ProveV0Manager* mgr)
    {
        Sha256 sha256;
        const std::string metaData =
            PowerV0::contentToMetaData(mgr->content);
        const Bigint K = sha256.doHash(
            metaData.c_str(), static_cast<u32>(metaData.size()));

        try
        {
            // Throws on error (not cancellation)
            RxManager rx(mgr->useLargePages, K, mgr->initCores,
                static_cast<u32>(mgr->hashCores.size()), mgr->cancelled);

            assert(mgr->state.load() == ProveV0Manager::State::rxIniting);

            if (mgr->cancelled.load())
            {
                std::lock_guard<std::mutex> lock(mgr->masterMutex);

                mgr->state.store(ProveV0Manager::State::rxCancelled);
                mgr->masterGuarded.rxTime = rx.initTime;
                mgr->masterGuarded.masterFinished = true;
            }
            else
            {
                std::optional<AbsTime> absTimeLimit;

                // Mutex scope
                {
                    std::lock_guard<std::mutex> lock(mgr->masterMutex);

                    mgr->masterGuarded.warnings = rx.warnings;
                    mgr->masterGuarded.rxTime = rx.initTime;
                    mgr->masterGuarded.hashStartTime.emplace(NOW);

                    if (mgr->timeLimit.has_value())
                        absTimeLimit.emplace(mgr->masterGuarded.hashStartTime.value() +
                            std::chrono::duration<double>(mgr->timeLimit.value()));

                    for (u32 t = 0; t < mgr->hashThreadCount; t++)
                    {
                        mgr->masterGuarded.bestResults.emplace_back();
                        mgr->threadGuarded.bestDiff.push_back(new std::atomic<u32>(0));
                        mgr->threadGuarded.hashes.push_back(new std::atomic<u64>(0));
                    }

                    for (u32 t = 0; t < mgr->hashThreadCount; t++)
                    {
                        mgr->hashThreads.emplace_back(hashThreadEntry, mgr, t, rx.getVM(t));
                        setThreadAffinity(mgr->hashThreads.back(), mgr->hashCores.at(t));
                    }

                    mgr->state.store(ProveV0Manager::State::hashing);
                    mgr->masterGuarded.threadsActive = true;
                }

                // Cond scope
                {
                    std::unique_lock<std::mutex> lock(mgr->masterMutex);

                    if (absTimeLimit.has_value())
                    {
                        mgr->masterCond.wait_until(lock, absTimeLimit.value(), [&]()
                        {
                            const NowTime predTime = NOW;

                            if (predTime >= absTimeLimit.value())
                            {
                                mgr->running.store(false);
                            }

                            assert(mgr->masterGuarded.threadsRunning <= mgr->hashThreadCount);
                            return (!mgr->running.load() || (mgr->masterGuarded.threadsRunning == 0));
                        });
                    }
                    else
                    {
                        mgr->masterCond.wait(lock, [&]()
                        {
                            assert(mgr->masterGuarded.threadsRunning <= mgr->hashThreadCount);
                            return (!mgr->running.load() || (mgr->masterGuarded.threadsRunning == 0));
                        });
                    }
                }

                assert(!mgr->running.load());
                const bool wasCancelled = mgr->isCancelled();
                
                for (u32 t = 0; t < mgr->hashThreadCount; t++)
                    mgr->hashThreads.at(t).join();

                // Mutex scope
                {
                    std::lock_guard<std::mutex> lock(mgr->masterMutex);

                    mgr->masterGuarded.hashStopTime.emplace(NOW);

                    if (wasCancelled)
                        mgr->state.store(ProveV0Manager::State::hashCancelled);
                    else
                        mgr->state.store(ProveV0Manager::State::finished);

                    mgr->masterGuarded.masterFinished = true;
                }
            }
        }
        catch (RxManager::Exception& e)
        {
            /* Thrown from RxManager c'tor - masterMutex must not be held in
               that context. */
            std::lock_guard<std::mutex> lock(mgr->masterMutex);

            mgr->masterGuarded.errorStr = e.getWhat();
            mgr->state.store(ProveV0Manager::State::rxFailed);
            mgr->masterGuarded.masterFinished = true;
        }
    }

    bool PowerV0::verifyMessage(
        const std::string& proof, bool useLargePages,
        std::optional<HashResult>& res, std::string& prettyMetaData,
        std::string& error)
    {
        bool ret = false;
        std::string proofTrimmed = proof;
        ProofContent content;

        prettyMetaData.clear();
        error.clear();
        trimBody(proofTrimmed);

        try
        {
            if (msgToContent(content, proofTrimmed))
            {
                /* Ret only indicates proof version correctness. Set now
                   in case of failure later. */
                ret = true;

                const std::string metaData = contentToMetaData(content);

                // Scope
                {
                    std::stringstream ss;

                    ss << "---- BEGIN BODY ----\n"
                        << content.body
                        << "\n----END BODY----\n"
                        << "\nUser ID: " << content.userId
                        << "\nContext: " << content.context;

                    prettyMetaData = ss.str();
                }

                Sha256 sha256;
                const Bigint K = sha256.doHash(
                    metaData.c_str(), static_cast<u32>(metaData.size()));

                RxManager rx(useLargePages, K);
                randomx_vm* const vm = rx.getVM(0);

                res.emplace();
                randomx_calculate_hash(
                    vm, content.body.c_str(), content.body.size(),
                    res->hash.getBytes());

                res->proof = content.body + '|' + metaData;
                res->diff = calcLZCDiff(res->hash);
            }
        }
        catch (RxManager::Exception& e)
        {
            error = e.getWhat();
        }

        return ret;
    }

    u32 PowerV0::calcLZCDiff(const Bigint& bigint)
    {
        const u8* buf = bigint.getBytes();

        for (u32 i = 0; i < 32; i++)
            for (u32 j = 0; j < 8; j++)
            {
                const u32 idx = i * 8 + j;
                const unsigned char diffmask = 1 << (7 - j);

                if (buf[i] & diffmask)
                    return idx;
            }

        return 256;

    }

    std::string PowerV0::contentToMetaData(const ProofContent& content)
    {
        std::string ret = "wxPoW0|";
        ret += content.userId;
        ret += '|';
        ret += content.context;

        return ret;
    }

    bool PowerV0::msgToContent(ProofContent& proof, const std::string& msgOrig)
    {
        bool ret = false;
        std::string msg = msgOrig;
        ProofContent out;

        trimBody(msg);

        const size_t msgLen = msg.size();
        const size_t pos = msg.rfind(magicSequence);

        
        if ((pos != std::string::npos) && (pos > 0))
        {
            out.body = msg.substr(0, pos);
            size_t idx = pos + magicSequence.size();

            bool consuming = true;
            bool justSawBackslash = false;

            if (idx < msgLen)
            {
                while (consuming && (idx < msgLen))
                {
                    const char c = msg.at(idx);

                    if (c == '\\')
                        justSawBackslash = true;
                    else if ((c == '|') && !justSawBackslash)
                        consuming = false;
                    else if ((c != '\\') || justSawBackslash)
                    {
                        out.userId += c;
                        justSawBackslash = false;
                    }

                    idx++;
                }

                justSawBackslash = false;

                while (idx < msgLen)
                {
                    const char c = msg.at(idx);

                    if (c == '\\')
                        justSawBackslash = true;
                    else if ((c != '\\') || justSawBackslash)
                    {
                        out.context += c;
                        justSawBackslash = false;
                    }

                    idx++;
                }

                proof = out;
                ret = true;
            }
        }

        return ret;
    }

    s32 PowerV0::utf8CharsToSkip(const std::string& str, s32 pos)
    {
        s32 ret = 0;
        const char c = str.at(pos);

        if ((c & 0x80) == 0)
        {
            if (std::binary_search(skippable1.cbegin(), skippable1.cend(), c))
                ret = 1;
        }
        else if ((c == static_cast<char>(0xc2)) && (static_cast<size_t>(pos + 1) <= str.size()))
        {
            const std::string substr2 = str.substr(pos, 2);

            if (std::binary_search(skippable2.cbegin(), skippable2.cend(), substr2))
                ret = 2;
        }
        else if (static_cast<size_t>(pos + 2) <= str.size())
        {
            const std::string substr3 = str.substr(pos, 3);

            if (std::binary_search(skippable3.cbegin(), skippable3.cend(), substr3))
                ret = 3;
        }

        return ret;
    }

    void PowerV0::trimBody(std::string& body)
    {
        const s32 msgLen = static_cast<s32>(body.size());
        s32 start = 0;

        if (msgLen > 0)
        {
            while (start < static_cast<s32>(body.size()))
            {
                const s32 skip = utf8CharsToSkip(body, start);

                if (skip > 0)
                    start += skip;
                else
                    break;
            }

            s32 end = static_cast<s32>(body.size() - 1);
            s32 charBytes = 0;

            while ((end - charBytes) >= start)
            {
                const char thisC = body.at(end - charBytes);

                if ((thisC & 0xc0) == 0x80)
                    charBytes++;
                else
                {
                    if ((end - charBytes) >= 0)
                    {
                        const s32 skip = utf8CharsToSkip(body, end - charBytes);

                        assert((skip == 0) || ((skip - 1) == charBytes));

                        if (skip > 0)
                        {
                            // Found trailing UTF-8 whitespace
                            end -= skip;
                            charBytes = 0;
                        }
                        else
                        {
                            // Reached a non-skippable UTF-8 char
                            break;
                        }
                    }
                    else
                    {
                        // Malformed string?
                        break;
                    }
                }
            }

            const s32 newLen = end - start + 1;
            assert(newLen >= 0);
            assert(newLen <= msgLen);

            if (newLen > 0)
            {
                body = body.substr(start, newLen);
                assert(newLen == static_cast<s32>(body.size()));
            }
            else
                body.clear();
        }
    }

    const std::string PowerV0::magicSequence = "|wxPoW0|";

    const std::vector<char> PowerV0::skippable1 {
        '\t', '\n', '\x0b', '\x0c', '\r', ' '
    };

    const std::vector<std::string> PowerV0::skippable2 {
        "\xc2\x85", "\xc2\xa0"
    };

    const std::vector<std::string> PowerV0::skippable3 {
        "\xe1\x9a\x80", "\xe1\xa0\x8e", "\xe2\x80\x80", "\xe2\x80\x81",
        "\xe2\x80\x82", "\xe2\x80\x83", "\xe2\x80\x84", "\xe2\x80\x85",
        "\xe2\x80\x86", "\xe2\x80\x87", "\xe2\x80\x88", "\xe2\x80\x89",
        "\xe2\x80\x8a", "\xe2\x80\x8b", "\xe2\x80\x8c", "\xe2\x80\x8d",
        "\xe2\x80\xa8", "\xe2\x80\xa9", "\xe2\x80\xaf", "\xe2\x81\x9f",
        "\xe2\x81\xa0", "\xe3\x80\x80", "\xe3\xbb\xbf"
    };

    RxManager::Exception::Exception(const std::string& what_) :
        what(what_)
    {}

    const std::string& RxManager::Exception::getWhat() const noexcept
    {
        return what;
    }

    RxManager::RxManager(
        bool useLargePages_, const Bigint& K_,
        const std::vector<u32>& initCores_, u32 hashCores_,
        const std::atomic<bool>& cancelled) :
        proveMode(true), useLargePages(useLargePages_), K(K_),
        initCores(initCores_), hashCores(hashCores_)
    {
        const auto tic = NOW;
        init();

        const u32 initThreads = static_cast<u32>(initCores.size());
        dataset = randomx_alloc_dataset(flags);

        if (dataset == nullptr)
        {
            std::string what = "Failed to initialize RX dataset.";

            if (useLargePages)
                what += " Try disabling large pages.";

            throw Exception(what);
        }

        const u32 datasetCount = randomx_dataset_item_count();
        const u32 countPerThread = datasetCount / initThreads;

        std::vector<std::thread> threads;

        for (u32 t = 0; t < (initThreads - 1); t++)
        {
            const u32 thisStart = countPerThread * t;

            threads.emplace_back(
                rxInitDatasetWrapper, t, dataset, cache,
                thisStart, countPerThread, std::cref(cancelled));

            if (!setThreadAffinity(threads.back(), initCores.at(t)))
            {
                char temp[128];

                snprintf(temp, 127, "Failed to set affinity for thread %"
                    PRIu32 " on core %" PRIu32,
                    t, initCores.at(t));
                temp[127] = 0;

                warnings.push_back(temp);
            }
        }

        const u32 lastStart = countPerThread * (initThreads - 1);
        const u32 lastCount = datasetCount - lastStart;

        threads.emplace_back(
            rxInitDatasetWrapper, initThreads - 1, dataset, cache,
            lastStart, lastCount, std::cref(cancelled));

        if (!setThreadAffinity(threads.back(), initCores.back()))
        {
            char temp[128];

            snprintf(temp, 127, "Failed to set affinity for thread %"
                PRIu32 " on core %" PRIu32,
                initThreads - 1, initCores.back());
            temp[127] = 0;

            warnings.push_back(temp);
        }

        for (u32 t = 0; t < initThreads; t++)
            threads.at(t).join();

        randomx_release_cache(cache);
        cache = nullptr;

        if (cancelled.load())
            warnings.push_back("RX initialization was cancelled.");
        else
            for (u32 i = 0; i < hashCores; i++)
                vms.push_back(randomx_create_vm(flags, cache, dataset));

        initTime.emplace(SECS(NOW - tic));
    }

    RxManager::RxManager(bool useLargePages_, const Bigint& K_) :
        proveMode(false), useLargePages(useLargePages_), K(K_), hashCores(1)
    {
        const auto tic = NOW;

        init();
        vms.push_back(randomx_create_vm(flags, cache, dataset));

        initTime.emplace(SECS(NOW - tic));
    }

    void RxManager::init()
    {
        flags = randomx_get_flags();

        if (proveMode)
            flags |= RANDOMX_FLAG_FULL_MEM;

        if (useLargePages)
            flags |= RANDOMX_FLAG_LARGE_PAGES;

        cache = randomx_alloc_cache(flags);

        if (cache == nullptr)
        {
            std::string what = "Failed to initialize RX cache.";

            if (useLargePages)
                what += " Try disabling large pages.";

            throw Exception(what);
        }

        randomx_init_cache(cache, K.getBytes(), 32);
    }

    RxManager::~RxManager()
    {
#ifndef NDEBUG
        if (proveMode)
        {
            assert(cache == nullptr);
            assert(dataset != nullptr);

        }
        else
        {
            assert(cache != nullptr);
            assert(dataset == nullptr);
        }
#endif

        if (dataset)
            randomx_release_dataset(dataset);

        if (cache)
            randomx_release_cache(cache);
    }

    randomx_vm* RxManager::getVM(u32 tid)
    {
        assert(vms.size() > 0);
        return vms.at(tid);
    }

    void RxManager::rxInitDatasetWrapper(
        u32 tid, randomx_dataset* dataset, randomx_cache* cache, unsigned long startItem,
        unsigned long itemCount, const std::atomic<bool>& cancelled)
    {
        for (unsigned long i = startItem; i < (startItem + itemCount); i++)
        {
            if ((i & 0xf) == (tid & 0xf))
                if (cancelled.load())
                    break;

            randomx_init_dataset(dataset, cache, i, 1);
        }
    }
}
