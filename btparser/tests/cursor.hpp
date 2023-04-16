struct TTD::Replay::IReplayEngine
{
    virtual void const * UnsafeAsInterface(struct _GUID const &) const = 0;
    virtual void * UnsafeAsInterface(struct _GUID const &) = 0;
    virtual enum Nirvana::GuestAddress GetPebAddress() const = 0;
    virtual struct TTD::SystemInfo const & GetSystemInfo() const = 0;
    virtual struct TTD::Replay::PositionRange const & GetLifetime() const = 0;
    virtual struct TTD::Replay::Position const & GetLastPosition() const = 0;
    virtual struct TTD::Replay::Position const & GetFirstPosition() const = 0;
    virtual enum TTD::Replay::RecordingType GetRecordingType() const = 0;
    virtual struct TTD::Replay::ThreadInfo const & GetThreadInfo(enum TTD::Replay::UniqueThreadId) const = 0;
    virtual uint64_t GetThreadCount() const = 0;
    virtual struct TTD::Replay::ThreadInfo const * GetThreadList() const = 0;
    virtual uint64_t const * GetThreadFirstPositionIndex() const = 0;
    virtual uint64_t const * GetThreadLastPositionIndex() const = 0;
    virtual uint64_t const * GetThreadLifetimeFirstPositionIndex() const = 0;
    virtual uint64_t const * GetThreadLifetimeLastPositionIndex() const = 0;
    virtual uint64_t GetThreadCreatedEventCount() const = 0;
    virtual struct TTD::Replay::ThreadCreatedEvent const * GetThreadCreatedEventList() const = 0;
    virtual uint64_t GetThreadTerminatedEventCount() const = 0;
    virtual struct TTD::Replay::ThreadTerminatedEvent const * GetThreadTerminatedEventList() const = 0;
    virtual uint64_t GetModuleCount() const = 0;
    virtual struct TTD::Replay::Module const * GetModuleList() const = 0;
    virtual uint64_t GetModuleInstanceCount() const = 0;
    virtual struct TTD::Replay::ModuleInstance const * GetModuleInstanceList() const = 0;
    virtual uint64_t const * GetModuleInstanceUnloadIndex() const = 0;
    virtual uint64_t GetModuleLoadedEventCount() const = 0;
    virtual struct TTD::Replay::ModuleLoadedEvent const * GetModuleLoadedEventList() const = 0;
    virtual uint64_t GetModuleUnloadedEventCount() const = 0;
    virtual struct TTD::Replay::ModuleUnloadedEvent const * GetModuleUnloadedEventList() const = 0;
    virtual uint64_t GetExceptionEventCount() const = 0;
    virtual struct TTD::Replay::ExceptionEvent const * GetExceptionEventList() const = 0;
    virtual struct TTD::Replay::ExceptionEvent const * GetExceptionAtOrAfterPosition(struct TTD::Replay::Position const &) const = 0;
    virtual uint64_t GetKeyframeCount() const = 0;
    virtual struct TTD::Replay::Position const * GetKeyframeList() const = 0;
    virtual uint64_t GetRecordClientCount() const = 0;
    virtual struct TTD::Replay::RecordClient const * GetRecordClientList() const = 0;
    virtual struct TTD::Replay::RecordClient const & GetRecordClient(enum TTD::RecordClientId) const = 0;
    virtual uint64_t GetCustomEventCount() const = 0;
    virtual struct TTD::Replay::CustomEvent const * GetCustomEventList() const = 0;
    virtual uint64_t GetActivityCount() const = 0;
    virtual struct TTD::Replay::Activity const * GetActivityList() const = 0;
    virtual uint64_t GetIslandCount() const = 0;
    virtual struct TTD::Replay::Island const * GetIslandList() const = 0;
    virtual class TTD::Replay::ICursor * NewCursor(struct _GUID const &) = 0;
    virtual enum TTD::Replay::IndexStatus BuildIndex(void (*)(void const *, struct TTD::Replay::IndexBuildProgressType const *), void const *, enum TTD::Replay::IndexBuildFlags) = 0;
    virtual enum TTD::Replay::IndexStatus GetIndexStatus() const = 0;
    virtual struct TTD::Replay::IndexFileStats GetIndexFileStats() const = 0;
    virtual void RegisterDebugModeAndLogging(enum TTD::Replay::DebugModeType, class TTD::ErrorReporting *) = 0;
    virtual class TTD::Replay::IEngineInternals const * GetInternals() const = 0;
    virtual class TTD::Replay::IEngineInternals * GetInternals() = 0;
    virtual ~IReplayEngine() = 0;
    virtual void Destroy() = 0;
    virtual bool Initialize(wchar_t const *) = 0;
};