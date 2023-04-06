struct TTD::Replay::IReplayEngine
{
    virtual void const * TTD::Replay::ReplayEngine::UnsafeAsInterface(struct _GUID const &) const = 0;
    virtual void * TTD::Replay::ReplayEngine::UnsafeAsInterface(struct _GUID const &) = 0;
    virtual enum Nirvana::GuestAddress TTD::Replay::ReplayEngine::GetPebAddress() const = 0;
    virtual struct TTD::SystemInfo const & TTD::Replay::ReplayEngine::GetSystemInfo() const = 0;
    virtual struct TTD::Replay::PositionRange const & TTD::Replay::ReplayEngine::GetLifetime() const = 0;
    virtual struct TTD::Replay::Position const & TTD::Replay::ReplayEngine::GetLastPosition() const = 0;
    virtual struct TTD::Replay::Position const & TTD::Replay::ReplayEngine::GetFirstPosition() const = 0;
    virtual enum TTD::Replay::RecordingType TTD::Replay::ReplayEngine::GetRecordingType() const = 0;
    virtual struct TTD::Replay::ThreadInfo const & TTD::Replay::ReplayEngine::GetThreadInfo(enum TTD::Replay::UniqueThreadId) const = 0;
    virtual uint64_t TTD::Replay::ReplayEngine::GetThreadCount() const = 0;
    virtual struct TTD::Replay::ThreadInfo const * TTD::Replay::ReplayEngine::GetThreadList() const = 0;
    virtual uint64_t const * TTD::Replay::ReplayEngine::GetThreadFirstPositionIndex() const = 0;
    virtual uint64_t const * TTD::Replay::ReplayEngine::GetThreadLastPositionIndex() const = 0;
    virtual uint64_t const * TTD::Replay::ReplayEngine::GetThreadLifetimeFirstPositionIndex() const = 0;
    virtual uint64_t const * TTD::Replay::ReplayEngine::GetThreadLifetimeLastPositionIndex() const = 0;
    virtual uint64_t TTD::Replay::ReplayEngine::GetThreadCreatedEventCount() const = 0;
    virtual struct TTD::Replay::ThreadCreatedEvent const * TTD::Replay::ReplayEngine::GetThreadCreatedEventList() const = 0;
    virtual uint64_t TTD::Replay::ReplayEngine::GetThreadTerminatedEventCount() const = 0;
    virtual struct TTD::Replay::ThreadTerminatedEvent const * TTD::Replay::ReplayEngine::GetThreadTerminatedEventList() const = 0;
    virtual uint64_t TTD::Replay::ReplayEngine::GetModuleCount() const = 0;
    virtual struct TTD::Replay::Module const * TTD::Replay::ReplayEngine::GetModuleList() const = 0;
    virtual uint64_t TTD::Replay::ReplayEngine::GetModuleInstanceCount() const = 0;
    virtual struct TTD::Replay::ModuleInstance const * TTD::Replay::ReplayEngine::GetModuleInstanceList() const = 0;
    virtual uint64_t const * TTD::Replay::ReplayEngine::GetModuleInstanceUnloadIndex() const = 0;
    virtual uint64_t TTD::Replay::ReplayEngine::GetModuleLoadedEventCount() const = 0;
    virtual struct TTD::Replay::ModuleLoadedEvent const * TTD::Replay::ReplayEngine::GetModuleLoadedEventList() const = 0;
    virtual uint64_t TTD::Replay::ReplayEngine::GetModuleUnloadedEventCount() const = 0;
    virtual struct TTD::Replay::ModuleUnloadedEvent const * TTD::Replay::ReplayEngine::GetModuleUnloadedEventList() const = 0;
    virtual uint64_t TTD::Replay::ReplayEngine::GetExceptionEventCount() const = 0;
    virtual struct TTD::Replay::ExceptionEvent const * TTD::Replay::ReplayEngine::GetExceptionEventList() const = 0;
    virtual struct TTD::Replay::ExceptionEvent const * TTD::Replay::ReplayEngine::GetExceptionAtOrAfterPosition(struct TTD::Replay::Position const &) const = 0;
    virtual uint64_t TTD::Replay::ReplayEngine::GetKeyframeCount() const = 0;
    virtual struct TTD::Replay::Position const * TTD::Replay::ReplayEngine::GetKeyframeList() const = 0;
    virtual uint64_t TTD::Replay::ReplayEngine::GetRecordClientCount() const = 0;
    virtual struct TTD::Replay::RecordClient const * TTD::Replay::ReplayEngine::GetRecordClientList() const = 0;
    virtual struct TTD::Replay::RecordClient const & TTD::Replay::ReplayEngine::GetRecordClient(enum TTD::RecordClientId) const = 0;
    virtual uint64_t TTD::Replay::ReplayEngine::GetCustomEventCount() const = 0;
    virtual struct TTD::Replay::CustomEvent const * TTD::Replay::ReplayEngine::GetCustomEventList() const = 0;
    virtual uint64_t TTD::Replay::ReplayEngine::GetActivityCount() const = 0;
    virtual struct TTD::Replay::Activity const * TTD::Replay::ReplayEngine::GetActivityList() const = 0;
    virtual uint64_t TTD::Replay::ReplayEngine::GetIslandCount() const = 0;
    virtual struct TTD::Replay::Island const * TTD::Replay::ReplayEngine::GetIslandList() const = 0;
    virtual class TTD::Replay::ICursor * TTD::Replay::ReplayEngine::NewCursor(struct _GUID const &) = 0;
    virtual enum TTD::Replay::IndexStatus TTD::Replay::ReplayEngine::BuildIndex(void (*)(void const *, struct TTD::Replay::IndexBuildProgressType const *), void const *, enum TTD::Replay::IndexBuildFlags) = 0;
    virtual enum TTD::Replay::IndexStatus TTD::Replay::ReplayEngine::GetIndexStatus() const = 0;
    virtual struct TTD::Replay::IndexFileStats TTD::Replay::ReplayEngine::GetIndexFileStats() const = 0;
    virtual void TTD::Replay::ReplayEngine::RegisterDebugModeAndLogging(enum TTD::Replay::DebugModeType, class TTD::ErrorReporting *) = 0;
    virtual class TTD::Replay::ICursorInternals * TTD::Replay::Cursor::GetInternals() = 0;
    virtual class TTD::Replay::IEngineInternals const * TTD::Replay::ReplayEngine::GetInternals() const = 0;
    virtual void * TTD::Replay::ReplayEngine::scalar_deleting_dtor(unsigned int) = 0;
    virtual void TTD::Replay::ReplayEngine::Destroy() = 0;
    virtual bool TTD::Replay::ReplayEngine::Initialize(wchar_t const *) = 0;
};