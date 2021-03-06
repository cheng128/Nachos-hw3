// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
// #include "synch.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------


void
ExceptionHandler(ExceptionType which)
{
	int	type = kernel->machine->ReadRegister(2);
	int	val, status;
	// Lock* memoryPagingLock = NULL;
    switch (which) {
		case SyscallException:
		{
			switch(type) {
			case SC_Halt:
				DEBUG(dbgAddr, "Shutdown, initiated by user program.\n");
				kernel->interrupt->Halt();
				break;
			case SC_PrintInt:
				val=kernel->machine->ReadRegister(4);
				cout << "Print integer:" <<val << endl;
				return;
	/*		case SC_Exec:
				DEBUG(dbgAddr, "Exec\n");
				val = kernel->machine->ReadRegister(4);
				kernel->StringCopy(tmpStr, retVal, 1024);
				cout << "Exec: " << val << endl;
				val = kernel->Exec(val);
				kernel->machine->WriteRegister(2, val);
				return;
	*/		case SC_Exit:
				DEBUG(dbgAddr, "Program exit\n");
				val=kernel->machine->ReadRegister(4);
				cout << "return value:" << val << endl;
				kernel->currentThread->Finish();
				break;

			case SC_Msg:
			{
				//DEBUG(dbgSys, "Message received.\n");
				val = kernel->machine->ReadRegister(4);
				{
					char *msg = &(kernel->machine->mainMemory[val]);
					cout << msg << endl;
				}
				kernel->interrupt->Halt();
				ASSERTNOTREACHED();
				break;
			}

			case SC_Create:
				val = kernel->machine->ReadRegister(4);
				{
					char *filename = &(kernel->machine->mainMemory[val]);
					status = kernel->fileSystem->Create(filename);	
					kernel->machine->WriteRegister(2, (int)status);
				}
				return;
				ASSERTNOTREACHED();
				break;
			default:
				cerr << "Unexpected system call " << type << "\n";
				break;
			}
		break;
		}
		case PageFaultException:
		{
			// // cout << "page fault exception" << endl;
			// int virtualAddr = kernel->machine->ReadRegister(BadVAddrReg);
			// // cout << "Bad Address: " << virtualAddr << endl;
			// unsigned int vpn = virtualAddr / PageSize;

			// // if (memoryPagingLock == NULL)
			// // memoryPagingLock = new Lock("memoryPagingLock");

			// // memoryPagingLock->Acquire();
			// kernel->currentThread->space->pageFault(vpn);
			// // memoryPagingLock->Release();
			return;
		}
		break;
		case AddressErrorException:
			cout << "Address Error Exception" << endl;
			break;		
		case ReadOnlyException:
			cout << "Read Only Exception" << endl;
			break;
		case BusErrorException:
			cout << "BusErrorException" << endl;
			break;
		default:
			cout << kernel->currentThread->getName() << endl;
			cout << "Bad Address: " << kernel->machine->ReadRegister(BadVAddrReg) << endl;
			cerr << "Unexpected user mode exception" << which << "\n";
			break;
	}
    ASSERTNOTREACHED();
}
