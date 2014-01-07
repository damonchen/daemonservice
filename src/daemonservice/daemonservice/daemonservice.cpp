#include <windows.h>


#define SERVICE_NANE		TEXT("DaemonService")

SERVICE_STATUS				g_service_status;
SERVICE_STATUS_HANDLE		g_service_status_handle;
HANDLE						g_service_stop_event = NULL;


void service_report_event(LPCTSTR error);
void report_service_status(DWORD current_state, DWORD win32_exit_code, DWORD wait_hint);
void service_init(DWORD dwArgs, LPTSTR *lpszArgv);
void WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv);
void WINAPI service_ctrl_handler(DWORD dwCtrl);



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


int main(int argc, char **argv)
{
	// 	if( strcmp(argv[1], "install") == 0 ){
	// 		service_install();
	// 		return 0;
	// 	}


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





