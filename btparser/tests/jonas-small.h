typedef struct _PROCESS_MITIGATION_USER_SHADOW_STACK_POLICY {
  union {
    DWORD Flags;
    struct {
      DWORD EnableUserShadowStack : 1;
      DWORD AuditUserShadowStack : 1;
      DWORD SetContextIpValidation : 1;
      DWORD AuditSetContextIpValidation : 1;
      DWORD EnableUserShadowStackStrictMode : 1;
      DWORD BlockNonCetBinaries : 1;
      DWORD BlockNonCetBinariesNonEhcont : 1;
      DWORD AuditBlockNonCetBinaries : 1;
      DWORD CetDynamicApisOutOfProcOnly : 1;
      DWORD SetContextIpValidationRelaxedMode : 1;
      DWORD ReservedFlags : 22;
    } DUMMYSTRUCTNAME;
  } DUMMYUNIONNAME;
} PROCESS_MITIGATION_USER_SHADOW_STACK_POLICY, *PPROCESS_MITIGATION_USER_SHADOW_STACK_POLICY;

typedef struct _PROCESS_MITIGATION_SIDE_CHANNEL_ISOLATION_POLICY {
    union {
        ULONG Flags;
        struct {

            //
            // Prevent branch target pollution cross-SMT-thread in user mode.
            //

            ULONG SmtBranchTargetIsolation : 1;

            //
            // Isolate this process into a distinct security domain, even from
            // other processes running as the same security context.  This
            // prevents branch target injection cross-process (normally such
            // branch target injection is only inhibited across different
            // security contexts).
            //
            // Page combining is limited to processes within the same security
            // domain.  This flag thus also effectively limits the process to
            // only being able to combine internally to the process itself,
            // except for common pages (unless further restricted by the
            // DisablePageCombine policy).
            //

            ULONG IsolateSecurityDomain : 1;

            //
            // Disable all page combining for this process, even internally to
            // the process itself, except for common pages (zeroes or ones).
            //

            ULONG DisablePageCombine : 1;

            //
            // Memory Disambiguation Disable.
            //

            ULONG SpeculativeStoreBypassDisable : 1;

            //
            // Prevent this process' threads from being scheduled on the same
            // core as threads outside its security domain.
            //

            ULONG RestrictCoreSharing : 1;

            ULONG ReservedFlags : 27;

        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} PROCESS_MITIGATION_SIDE_CHANNEL_ISOLATION_POLICY, *PPROCESS_MITIGATION_SIDE_CHANNEL_ISOLATION_POLICY;

