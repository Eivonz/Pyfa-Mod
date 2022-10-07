extern g_AccGetRunningUtilityState: DQ
extern g_AccNotifyTouchInteraction: DQ
extern g_AccSetRunningUtilityState: DQ
extern g_AccessibleChildren: DQ
extern g_AccessibleObjectFromEvent: DQ
extern g_AccessibleObjectFromPoint: DQ
extern g_AccessibleObjectFromWindow: DQ
extern g_AccessibleObjectFromWindowTimeout: DQ
extern g_CreateStdAccessibleObject: DQ
extern g_CreateStdAccessibleProxyA: DQ
extern g_CreateStdAccessibleProxyW: DQ
extern g_DllCanUnloadNow: DQ
extern g_DllGetClassObject: DQ
extern g_DllRegisterServer: DQ
extern g_DllUnregisterServer: DQ
extern g_GetOleaccVersionInfo: DQ
extern g_GetProcessHandleFromHwnd: DQ
extern g_GetRoleTextA: DQ
extern g_GetRoleTextW: DQ
extern g_GetStateTextA: DQ
extern g_GetStateTextW: DQ
extern g_IID_IAccessible: DQ
extern g_IID_IAccessibleHandler: DQ
extern g_LIBID_Accessibility: DQ
extern g_LresultFromObject: DQ
extern g_ObjectFromLresult: DQ
extern g_PropMgrClient_LookupProp: DQ
extern g_WindowFromAccessibleObject: DQ

.code
FakeAccGetRunningUtilityState proc
        jmp g_AccGetRunningUtilityState
FakeAccGetRunningUtilityState endp
FakeAccNotifyTouchInteraction proc
        jmp g_AccNotifyTouchInteraction
FakeAccNotifyTouchInteraction endp
FakeAccSetRunningUtilityState proc
        jmp g_AccSetRunningUtilityState
FakeAccSetRunningUtilityState endp
FakeAccessibleChildren proc
        jmp g_AccessibleChildren
FakeAccessibleChildren endp
FakeAccessibleObjectFromEvent proc
        jmp g_AccessibleObjectFromEvent
FakeAccessibleObjectFromEvent endp
FakeAccessibleObjectFromPoint proc
        jmp g_AccessibleObjectFromPoint
FakeAccessibleObjectFromPoint endp
FakeAccessibleObjectFromWindow proc
        jmp g_AccessibleObjectFromWindow
FakeAccessibleObjectFromWindow endp
FakeAccessibleObjectFromWindowTimeout proc
        jmp g_AccessibleObjectFromWindowTimeout
FakeAccessibleObjectFromWindowTimeout endp
FakeCreateStdAccessibleObject proc
        jmp g_CreateStdAccessibleObject
FakeCreateStdAccessibleObject endp
FakeCreateStdAccessibleProxyA proc
        jmp g_CreateStdAccessibleProxyA
FakeCreateStdAccessibleProxyA endp
FakeCreateStdAccessibleProxyW proc
        jmp g_CreateStdAccessibleProxyW
FakeCreateStdAccessibleProxyW endp
FakeDllCanUnloadNow proc
        jmp g_DllCanUnloadNow
FakeDllCanUnloadNow endp
FakeDllGetClassObject proc
        jmp g_DllGetClassObject
FakeDllGetClassObject endp
FakeDllRegisterServer proc
        jmp g_DllRegisterServer
FakeDllRegisterServer endp
FakeDllUnregisterServer proc
        jmp g_DllUnregisterServer
FakeDllUnregisterServer endp
FakeGetOleaccVersionInfo proc
        jmp g_GetOleaccVersionInfo
FakeGetOleaccVersionInfo endp
FakeGetProcessHandleFromHwnd proc
        jmp g_GetProcessHandleFromHwnd
FakeGetProcessHandleFromHwnd endp
FakeGetRoleTextA proc
        jmp g_GetRoleTextA
FakeGetRoleTextA endp
FakeGetRoleTextW proc
        jmp g_GetRoleTextW
FakeGetRoleTextW endp
FakeGetStateTextA proc
        jmp g_GetStateTextA
FakeGetStateTextA endp
FakeGetStateTextW proc
        jmp g_GetStateTextW
FakeGetStateTextW endp
FakeIID_IAccessible proc
        jmp g_IID_IAccessible
FakeIID_IAccessible endp
FakeIID_IAccessibleHandler proc
        jmp g_IID_IAccessibleHandler
FakeIID_IAccessibleHandler endp
FakeLIBID_Accessibility proc
        jmp g_LIBID_Accessibility
FakeLIBID_Accessibility endp
FakeLresultFromObject proc
        jmp g_LresultFromObject
FakeLresultFromObject endp
FakeObjectFromLresult proc
        jmp g_ObjectFromLresult
FakeObjectFromLresult endp
FakePropMgrClient_LookupProp proc
        jmp g_PropMgrClient_LookupProp
FakePropMgrClient_LookupProp endp
FakeWindowFromAccessibleObject proc
        jmp g_WindowFromAccessibleObject
FakeWindowFromAccessibleObject endp
end