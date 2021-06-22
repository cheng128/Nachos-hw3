// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -n -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "addrspace.h"
#include "machine.h"
#include "noff.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//----------------------------------------------------------------------

AddrSpace::AddrSpace()
{
//     for(unsigned int i = 0; i < NumPhysPages; i++)
//         AddrSpace::PhyPageStatus[i] = FALSE;
//     AddrSpace::NumFreePhyPages = NumPhysPages;

//     pageTable = new TranslationEntry[NumPhysPages];
//     for (unsigned int i = 0; i < NumPhysPages; i++) {
// 	pageTable[i].virtualPage = i;	// for now, virt page # = phys page #
// 	pageTable[i].physicalPage = i;
// //	pageTable[i].physicalPage = 0;
// 	// pageTable[i].valid = TRUE;
// 	pageTable[i].valid = FALSE;
// 	pageTable[i].use = FALSE;
// 	pageTable[i].dirty = FALSE;
// 	pageTable[i].readOnly = FALSE;  
//     }
    // zero out the entire address space
//    bzero(kernel->machine->mainMemory, MemorySize);
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    kernel->fileSystem->Remove(strcat(kernel->currentThread->getName(), "_vm"));
    delete pageTable;
}


//----------------------------------------------------------------------
// AddrSpace::Load
// 	Load a user program into memory from a file.
//
//	Assumes that the page table has been initialized, and that
//	the object code file is in NOFF format.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

bool 
AddrSpace::Load(char *fileName) 
{
    OpenFile *executable = kernel->fileSystem->Open(fileName);
    NoffHeader noffH;
    unsigned int size;

    if (executable == NULL) {
	cerr << "Unable to open file " << fileName << "\n";
	return FALSE;
    }
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    // size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
	// 		+ UserStackSize;	// we need to increase the size
	// 					        // to leave room for the stack
    size = 16384;
    numPages = divRoundUp(size, PageSize);
    
    
    size = numPages * PageSize;

    // ASSERT(numPages <= NumFreePhyPages);		// check we're not trying
    //                                             // to run anything too big --
    //                                             // at least until we have
    //                                             // virtual memory
    cout << "size: " << size << endl;
    cout << "numPages: " << numPages << endl; 
    pageTable = new TranslationEntry[numPages];
    for(unsigned int i = 0, idx = 0; i < numPages; i++) {
        pageTable[i].virtualPage = i;
        while(idx < NumPhysPages-1 && kernel->machine->PhyPageStatus[idx] == TRUE) idx++;
        pageTable[i].virtualPage = i;
        pageTable[i].physicalPage = i;
        pageTable[i].valid = FALSE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;
    }

    DEBUG(dbgAddr, "Initializing address space: " << numPages << ", " << size);
    
// then, copy in the code and data segments into memory
	if (noffH.code.size > 0) {
        DEBUG(dbgAddr, "Initializing code segment.");
	    DEBUG(dbgAddr, noffH.code.virtualAddr << ", " << noffH.code.size);
        char *buf1;
        buf1 = new char[noffH.code.size];
        int a = executable->ReadAt(buf1, noffH.code.size, noffH.code.inFileAddr);
        // cout << "Load executable: " << a << endl;
        int b = kernel->currentThread->space->vm->WriteAt(buf1, noffH.code.size, noffH.code.virtualAddr);
        // cout << "after write vm code: " << b << endl;
    }

	if (noffH.initData.size > 0) {
        DEBUG(dbgAddr, "Initializing data segment.");
	    DEBUG(dbgAddr, noffH.initData.virtualAddr << ", " << noffH.initData.size);
        char *buf2;
        buf2 = new char[noffH.initData.size];
        int a = executable->ReadAt(buf2, noffH.initData.size, noffH.initData.inFileAddr);
        // cout << "Load executable init data: " << a << endl;
        int b = kernel->currentThread->space->vm->WriteAt(buf2, noffH.initData.size, noffH.initData.virtualAddr);
        // cout << "after write vm init: " << b << endl;
    }

    delete executable;			// close file
    return TRUE;			// success
}

//----------------------------------------------------------------------
// AddrSpace::Execute
// 	Run a user program.  Load the executable into memory, then
//	(for now) use our own thread to run it.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

void 
AddrSpace::Execute(char *fileName) 
{   
    if (!Load(fileName)) {
	return;				// executable not found
    }

    //kernel->currentThread->space = this;
    this->InitRegisters();		// set the initial register values
    this->RestoreState();		// load page table register

    kernel->machine->Run();		// jump to the user progam

    ASSERTNOTREACHED();			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}


//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    Machine *machine = kernel->machine;
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG(dbgAddr, "Initializing stack pointer: " << numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, don't need to save anything!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{
        pageTable=kernel->machine->pageTable;
        numPages=kernel->machine->pageTableSize;
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    kernel->machine->pageTable = pageTable;
    kernel->machine->pageTableSize = numPages;
}

//<HW3
int AddrSpace::pageFault(int vpn)
{
    // cout << "in pageFault function: vpn = " << vpn << endl;
    kernel->stats->numPageFaults ++;
    cout << "in pageFault before into AllocPage" << endl;
    pageTable[vpn].physicalPage = AllocPage(this, vpn);
    cout << "in pageFatulte after AllocPage find physicalpage: " << pageTable[vpn].physicalPage << endl;
    
    // cout << "pageTable[vpn].physicalPage: " << pageTable[vpn].physicalPage << endl;
    loadPage(vpn);
	// cout << "from load page back to pagefault" << endl;
    // cout << "vpn: " << vpn << " pageTable[vpn].virtualPage: " << pageTable[vpn].virtualPage << endl;
	pageTable[vpn].valid = TRUE;
	pageTable[vpn].use = FALSE;
	pageTable[vpn].dirty = FALSE;
	pageTable[vpn].readOnly = FALSE;
	// cout << "end of pagefault" << endl;

	return 0;
}


int AddrSpace::AllocPage(AddrSpace* space, int vpn)
{   
    cout << "in AllocPage" << endl;
    kernel->lock->Acquire();
    int physNum = FindFreePage();
    // cout << "Alloc: PhysNum after FindFree: " << physNum << endl;
    if (physNum == -1)
    {
        physNum = FindVictim();
        cout << "Alloc: victim: " << physNum << endl;
        cout << "current Thread: " << kernel->currentThread->getName() << endl;
        kernel->UsedProcess[physNum]->evictPage(kernel->invertTable[physNum]);
    }

    kernel->UsedProcess[physNum] = space;
    // cout << "in Alloc Page new kernel->UsedProcess[physNum]: " <<  kernel->UsedProcess[physNum] << endl;
    // cout << "current thread: " << kernel->currentThread->getName() << endl;
    kernel->invertTable[physNum] = vpn;
    // cout << "before return physNum: " << physNum << endl;
    kernel->lock->Release();
    return physNum;
}

int AddrSpace::FindFreePage()
{
    // cout << "in FindFreePage function" << endl;
    for(unsigned int i=0; i<NumPhysPages; i++)
    {
        if(kernel->machine->PhyPageStatus[i]==FALSE)
        {
            cout << "find Free Page: " << i << endl;
            kernel->machine->PhyPageStatus[i] = TRUE;
            return i;
        }
    }
    return -1;
}

int AddrSpace::FindVictim()
{
    // cout << "in FindVictim function" << endl;
    unsigned int ppn = rand() % NumPhysPages;
    // cout << "Find victim: " << kernel->UsedProcess[ppn] << endl;
    return ppn;
}

int  AddrSpace::loadPage(int vpn)
{
    // cout << "in loadPage" << endl;
    // cout << "vpn: " << vpn << endl;
    // cout << "pageTable[vpn].physicalPage: " << pageTable[vpn].physicalPage << endl;
    bzero(&kernel->machine->mainMemory[pageTable[vpn].physicalPage * PageSize], PageSize);
    int a = kernel->currentThread->space->vm->ReadAt(&kernel->machine->mainMemory[pageTable[vpn].physicalPage * PageSize],
                                                    PageSize,
                                                    pageTable[vpn].virtualPage * PageSize);

    // cout << "loadPage phy address: " << pageTable[vpn].physicalPage * PageSize << "   a: " << a << endl;
    return 0;
}

int AddrSpace::evictPage(int vpn)
{
    cout << "in evictPage" << endl;
    cout << "evict Thread: " << kernel->currentThread->getName() << endl;
    if(pageTable[vpn].dirty)
    {
        SwapOut(vpn);
    }

    pageTable[vpn].physicalPage = -1;
	pageTable[vpn].valid = FALSE;
	pageTable[vpn].use = FALSE;
	pageTable[vpn].dirty = FALSE;

    return 0;
}

int AddrSpace::SwapOut(int vpn)
{
    // cout << "in SwapOut function" << endl;
    // cout << "swap out phy page: " << pageTable[vpn].physicalPage << endl;
    // cout << "swap out phy address: " << pageTable[vpn].physicalPage * PageSize << endl;
    // cout << "swap out valid: " << pageTable[vpn].valid << endl;
    // cout << "pageTable[vpn].virtualPage * PageSize: " << pageTable[vpn].virtualPage * PageSize << endl;
    int a = kernel->currentThread->space->vm->WriteAt(&kernel->machine->mainMemory[pageTable[vpn].physicalPage * PageSize],
                                                        PageSize,
                                                        pageTable[vpn].virtualPage * PageSize);
    // cout << "swap out: " << a << endl;
    return 0;
}  
//HW3>