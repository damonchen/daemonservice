#include <windows.h>
#include <tchar.h>

#define SERVICE_NANE		TEXT("DaemonService")

SERVICE_STATUS				g_service_status;
SERVICE_STATUS_HANDLE		g_service_status_handle;
HANDLE						g_service_stop_event = NULL;


void service_report_event(LPCTSTR error);
void report_service_status(DWORD current_state, DWORD win32_exit_code, DWORD wait_hint);
void service_init(DWORD dwArgs, LPTSTR *lpszArgv);
void WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv);
void WINAPI service_ctrl_handler(DWORD dwCtrl);
void service_remove();
void service_install();
void service_restart();
void service_stop();
void service_start();



void WINAPI service_ctrl_handler(DWORD dwCtrl)
{
	switch(dwCtrl){
		case SERVICE_CONTROL_STOP:
			report_service_status(SERVICE_STOP_PENDING, NO_ERROR, 0);
			SetEvent(g_service_stop_event);
			report_service_status(g_service_status.dwCurrentState, NO_ERROR, 0);
		case SERVICE_CONTROL_INTERROGATE:
			break;
		default:
			break;
	}
}


void WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv)
{
	g_service_status_handle = RegisterServiceCtrlHandler(
		SERVICE_NANE, service_ctrl_handler
	);

	if( !g_service_status_handle ){
		service_report_event(TEXT("RegisterServiceCtrlHandler"));
		return ;
	}

	g_service_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_service_status.dwServiceSpecificExitCode = 0;

	// report inital status to the SCM
	report_service_status(SERVICE_START_PENDING, NO_ERROR, 3000);

	// perform service-specific initalization and work
	service_init(dwArgc, lpszArgv);

}

void report_service_status(DWORD current_state, DWORD win32_exit_code, DWORD wait_hint)
{
	static DWORD check_point = 1;

	g_service_status.dwCurrentState = current_state;
	g_service_status.dwWin32ExitCode = win32_exit_code;
	g_service_status.dwWaitHint = wait_hint;

	if( current_state == SERVICE_START_PENDING ){
		g_service_status.dwControlsAccepted = 0;
	}else{
		g_service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	}

	if( (current_state == SERVICE_RUNNING) || 
		(current_state == SERVICE_STOPPED) ){
			g_service_status.dwCheckPoint = 0;
	}else{
		g_service_status.dwCheckPoint = check_point++;
	}

	SetServiceStatus(g_service_status_handle, &g_service_status);
}


void service_init(DWORD dwArgs, LPTSTR *lpszArgv)
{
	g_service_stop_event = CreateEvent(NULL, TRUE, FALSE, NULL);
	if( g_service_stop_event == NULL ){
		report_service_status(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}

	report_service_status(SERVICE_RUNNING, NO_ERROR, 0);

	while(TRUE){
		WaitForSingleObject(g_service_stop_event, INFINITE);
		report_service_status(SERVICE_STOPPED, NO_ERROR, 0);
	}
}


void service_report_event(LPCTSTR error)
{

}

void service_install()
{
	// get  the file name of process
	TCHAR filename[MAX_PATH + 2] = {0};
	TCHAR *start = filename + 1;		
	INT size = GetModuleFileName(NULL, start, MAX_PATH);
	if( size <= 0 ){
		service_report_event(TEXT("GetModuleFileName"));
		return ;
	}

	// adjust file name for space.
	BOOL is_space_exist = _tcschr(filename, TEXT(' ')) != NULL;
	if( is_space_exist ){
		filename[0] = TEXT('\"');
		filename[size] = TEXT('\"');
		start = filename;
	}

	// get a service control manager object handle
	SC_HANDLE sch = OpenSCManager(NULL, NULL,  SC_MANAGER_CREATE_SERVICE);
	if( sch == NULL ){
		service_report_event(TEXT("OpenSCManager"));
		return ;
	}

	SC_HANDLE sccs = CreateService(sch, SERVICE_NANE, SERVICE_NANE, SC_MANAGER_CREATE_SERVICE,
					SERVICE_WIN32_OWN_PROCESS| SERVICE_INTERACTIVE_PROCESS, SERVICE_AUTO_START, 
					SERVICE_ERROR_NORMAL, start, NULL, NULL, NULL, NULL, NULL);
	if( sccs == NULL ){
		service_report_event(TEXT("CreateService"));
		CloseServiceHandle(sch);
		return ;
	}

	CloseServiceHandle(sccs);
	CloseServiceHandle(sch);
}


void service_remove()
{
	SC_HANDLE sch = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if( sch == NULL ){
		service_report_event(TEXT("OpenSCManager"));
		return ;
	}

	SC_HANDLE sccs = OpenService(sch, SERVICE_NANE, DELETE);
	if( sccs == NULL ){
		service_report_event(TEXT("OpenService"));
		CloseServiceHandle(sch);
		return ;
	}

	if( !DeleteService(sccs) ){
		service_report_event(TEXT("DeleteService"));
	}

	CloseServiceHandle(sccs);
	CloseServiceHandle(sch);
}


void service_restart()
{
	service_stop();
	service_start();
}

void service_stop()
{
	SC_HANDLE sch = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if( sch == NULL ){
		service_report_event(TEXT("OpenSCManager"));
		return ;
	}

	SC_HANDLE sccs = OpenService(sch, SERVICE_NANE, SERVICE_STOP);
	if( sccs == NULL ){
		service_report_event(TEXT("OpenService"));
		CloseServiceHandle(sch);
		return ;
	}

	SERVICE_STATUS ss;
	if ( !ControlService(sccs, SERVICE_CONTROL_STOP, &ss) ){
		service_report_event(TEXT("ControlService"));
	}

	CloseServiceHandle(sccs);
	CloseServiceHandle(sch);
}


void service_start()
{
	SC_HANDLE sch = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if( sch == NULL ){
		service_report_event(TEXT("OpenSCManager"));
		return ;
	}

	SC_HANDLE sccs = OpenService(sch, SERVICE_NANE, SERVICE_START);
	if( sccs == NULL ){
		service_report_event(TEXT("OpenService"));
		CloseServiceHandle(sch);
		return ;
	}

	if ( !StartService(sch, NULL, NULL) ){
		service_report_event(TEXT("StartService"));
	}

	CloseServiceHandle(sccs);
	CloseServiceHandle(sch);
}




int main(int argc, char **argv)
{
	if( strcmp(argv[1], "install") == 0 ){
		service_install();
		return 0;
	}

	if( strcmp(argv[1], "remove") == 0 ){
		service_remove();
	}

	if ( strcmp(argv[1], "restart") == 0 ){
		service_restart();
	}

	if ( strcmp(argv[1], "stop") == 0 ) {
		service_stop();
	}

	if ( strcmp(argv[1], "start") == 0 ){
		service_start();
	}

	SERVICE_TABLE_ENTRY service_table_entry[] = {
		{
			SERVICE_NANE, (LPSERVICE_MAIN_FUNCTION)service_main,
		},
		{
			NULL, NULL,
		},
	};

	if( !StartServiceCtrlDispatcher(service_table_entry) ){
		service_report_event(TEXT("StartServiceCtrlDispatcher"));
	}
}

