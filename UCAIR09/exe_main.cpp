#include <iostream>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include "main.h"

#ifdef WIN32

#ifdef DEBUG
#else
#include "windows.h"
#endif

using namespace std;
using namespace boost;

function0<void> stop_handler;

BOOL WINAPI console_ctrl_handler(DWORD ctrl_type){
	switch (ctrl_type){
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		stop_handler();
		return TRUE;
	default:
		return FALSE;
	}
}

// If we use WinMain, we won't see console window.
#ifdef DEBUG
int main(int argc, char *argv[]) {
#else
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	int argc = 1;
	char *argv[2];
	argv[0] = GetCommandLine();
	argv[1] = NULL;
#endif
	ucair::Main m(argc, argv);
	stop_handler = bind(&ucair::Main::interrupt, &m);
	SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
	m.start();

	return 0;
}

#else

#include <pthread.h>
#include <signal.h>

using namespace std;
using namespace boost;

int main(int argc, char *argv[]){
	try{
		ucair::Main m(argc, argv);

		// Block all signals for background thread.
		sigset_t new_mask;
		sigfillset(&new_mask);
		sigset_t old_mask;
		pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

		thread t(bind(&ucair::Main::start, &m));

		// Restore previous signals.
		pthread_sigmask(SIG_SETMASK, &old_mask, 0);

		// Wait for signal indicating time to shut down.
		sigset_t wait_mask;
		sigemptyset(&wait_mask);
		sigaddset(&wait_mask, SIGINT);
		sigaddset(&wait_mask, SIGQUIT);
		sigaddset(&wait_mask, SIGTERM);
		pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
		int sig = 0;
		sigwait(&wait_mask, &sig);

		m.interrupt();
		t.join();
	}
	catch (std::exception& e){
		cerr << "exception: " << e.what() << "\n";
	}

	return 0;
}

#endif
