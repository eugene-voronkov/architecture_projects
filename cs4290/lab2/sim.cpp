#include "common.h"
#include "sim.h"
#include "trace.h" 
#include "cache.h"  /**** NEW-LAB2*/ 
#include "memory.h" // NEW-LAB2 
#include <stdlib.h>
#include <ctype.h> /* Library for useful character operations */

/*
------------------------------MY STUFF
*/
#ifdef DEBUG_ON
 #define DEBUG if(1) 
#else
 #define DEBUG if(0) 
#endif


#ifdef RUN_MEM
 #define MEMTEST_OFF if(0)
 #define MEMTEST_ON if(1)
#else
 #define MEMTEST_OFF if(1)
 #define MEMTEST_ON if(0) 
#endif

extern int request_count;

void print_pipe();
void init_registers(void);
void memory_test(memory_c *);
/*
-----------------------------END OF MY STUFF
*/

/*******************************************************************/
/* Simulator frame */ 
/*******************************************************************/

bool run_a_cycle(memory_c *m ); // please modify run_a_cycle function argument  /** NEW-LAB2 */ 
void init_structures(memory_c *m); // please modify init_structures function argument  /** NEW-LAB2 */ 



/* uop_pool related variables */ 

uint32_t free_op_num;
uint32_t active_op_num; 
Op *op_pool; 
Op *op_pool_free_head = NULL; 

/* simulator related local functions */ 

bool icache_access(ADDRINT addr); /** please change uint32_t to ADDRINT NEW-LAB2 */  
bool dcache_access(ADDRINT addr); /** please change uint32_t to ADDRINT NEW-LAB2 */   
void  init_latches(void);

#include "knob.h"
#include "all_knobs.h"

// knob variables
KnobsContainer *g_knobsContainer; /* < knob container > */
all_knobs_c    *g_knobs; /* < all knob variables > */

gzFile g_stream;

void init_knobs(int argc, char** argv)
{
  // Create the knob managing class
  g_knobsContainer = new KnobsContainer();

  // Get a reference to the actual knobs for this component instance
  g_knobs = g_knobsContainer->getAllKnobs();

  // apply the supplied command line switches
  char* pInvalidArgument = NULL;
  g_knobsContainer->applyComandLineArguments(argc, argv, &pInvalidArgument);

  g_knobs->display();
}

void read_trace_file(void)
{
  g_stream = gzopen((KNOB(KNOB_TRACE_FILE)->getValue()).c_str(), "r");
}

// simulator main function is called from outside of this file 

void simulator_main(int argc, char** argv) 
{
  init_knobs(argc, argv);

  // trace driven simulation 
  read_trace_file();

  /** NEW-LAB2 */ /* just note: passing main memory pointers is hack to mix up c++ objects and c-style code */  /* Not recommended at all */ 
  memory_c *main_memory = new memory_c();  // /** NEW-LAB2 */ 


  init_structures(main_memory);  // please modify run_a_cycle function argument  /** NEW-LAB2 */ 
  MEMTEST_OFF run_a_cycle(main_memory); // please modify run_a_cycle function argument  /** NEW-LAB2 */ 
  MEMTEST_ON memory_test(main_memory);

}
int op_latency[NUM_OP_TYPE]; 

void init_op_latency(void)
{
  op_latency[OP_INV]   = 1; 
  op_latency[OP_NOP]   = 1; 
  op_latency[OP_CF]    = 1; 
  op_latency[OP_CMOV]  = 1; 
  op_latency[OP_LDA]   = 1;
  op_latency[OP_LD]    = 1; 
  op_latency[OP_ST]    = 1; 
  op_latency[OP_IADD]  = 1; 
  op_latency[OP_IMUL]  = 2; 
  op_latency[OP_IDIV]  = 4; 
  op_latency[OP_ICMP]  = 2; 
  op_latency[OP_LOGIC] = 1; 
  op_latency[OP_SHIFT] = 2; 
  op_latency[OP_BYTE]  = 1; 
  op_latency[OP_MM]    = 2; 
  op_latency[OP_FMEM]  = 2; 
  op_latency[OP_FCF]   = 1; 
  op_latency[OP_FCVT]  = 4; 
  op_latency[OP_FADD]  = 2; 
  op_latency[OP_FMUL]  = 4; 
  op_latency[OP_FDIV]  = 16; 
  op_latency[OP_FCMP]  = 2; 
  op_latency[OP_FBIT]  = 2; 
  op_latency[OP_FCMP]  = 2; 
}

void init_op(Op *op)
{
  op->num_src               = 0; 
  op->src[0]                = -1; 
  op->src[1]                = -1;
  op->dst                   = -1; 
  op->opcode                = 0; 
  op->is_fp                 = false;
  op->cf_type               = NOT_CF;
  op->mem_type              = NOT_MEM;
  op->write_flag             = 0;
  op->inst_size             = 0;
  op->ld_vaddr              = 0;
  op->st_vaddr              = 0;
  op->instruction_addr      = 0;
  op->branch_target         = 0;
  op->actually_taken        = 0;
  op->mem_read_size         = 0;
  op->mem_write_size        = 0;
  op->valid                 = FALSE; 
  /* you might add more features here */ 
}


void init_op_pool(void)
{
  /* initialize op pool */ 
  op_pool = new Op [1024];
  free_op_num = 1024; 
  active_op_num = 0; 
  uint32_t op_pool_entries = 0; 
  int ii;
  for (ii = 0; ii < 1023; ii++) {

    op_pool[ii].op_pool_next = &op_pool[ii+1]; 
    op_pool[ii].op_pool_id   = op_pool_entries++; 
    init_op(&op_pool[ii]); 
  }
  op_pool[ii].op_pool_next = op_pool_free_head; 
  op_pool[ii].op_pool_id   = op_pool_entries++;
  init_op(&op_pool[ii]); 
  op_pool_free_head = &op_pool[0]; 
}


Op *get_free_op(void)
{
  /* return a free op from op pool */ 

  if (op_pool_free_head == NULL || (free_op_num == 1)) {
    std::cout <<"ERROR! OP_POOL SIZE is too small!! " << endl; 
    std::cout <<"please check free_op function " << endl; 
    assert(1); 
    exit(1);
  }

  free_op_num--;
  assert(free_op_num); 

  Op *new_op = op_pool_free_head; 
  op_pool_free_head = new_op->op_pool_next; 
  //test code
  if(new_op->valid) {
	printf( "valid new_op instr: %d\n", new_op->inst_id );
	}
  assert(!new_op->valid); 
  init_op(new_op);
  active_op_num++; 
  return new_op; 
}

void free_op(Op *op)
{
  free_op_num++;
  active_op_num--; 
  op->valid = FALSE; 
  op->op_pool_next = op_pool_free_head;
  op_pool_free_head = op; 
}



/*******************************************************************/
/*  Data structure */
/*******************************************************************/

typedef struct pipeline_latch_struct {
  Op *op; /* you must update this data structure. */
  bool op_valid; 
   /* you might add more data structures. But you should complete the above data elements */ 
}pipeline_latch; 


typedef struct Reg_element_struct{
  bool valid;
  // data is not needed 
  /* you might add more data structures. But you should complete the above data elements */ 
}REG_element; 

REG_element register_file[NUM_REG];


/*******************************************************************/
/* These are the functions you'll have to write.  */ 
/*******************************************************************/

void FE_stage();
void ID_stage();
void EX_stage(); 
void MEM_stage(memory_c *main_memory); // please modify MEM_stage function argument  /** NEW-LAB2 */ 
void WB_stage(); 

/*******************************************************************/
/*  These are the variables you'll have to write.  */ 
/*******************************************************************/

bool sim_end_condition = FALSE;     /* please complete the condition. */ 
UINT64 retired_instruction = 0;    /* number of retired instruction. (only correct instructions) */ 
UINT64 cycle_count = 0;            /* total number of cycles */ 
UINT64 data_hazard_count = 0;  
UINT64 control_hazard_count = 0; 
UINT64 icache_miss_count = 0;      /* total number of icache misses. for Lab #2 and Lab #3 */ 
UINT64 dcache_hit_count = 0;      /* total number of dcache  misses. for */
UINT64 dcache_miss_count = 0;      /* total number of dcache  misses. for Lab #2 and Lab #3 */ 
UINT64 l2_cache_miss_count = 0;    /* total number of L2 cache  misses. for Lab #2 and Lab #3 */  
UINT64 dram_row_buffer_hit_count = 0; /* total number of dram row buffer hit. for Lab #2 and Lab #3 */   // NEW-LAB2
UINT64 dram_row_buffer_miss_count = 0; /* total number of dram row buffer hit. for Lab #2 and Lab #3 */   // NEW-LAB2
UINT64 store_load_forwarding_count = 0;  /* total number of store load forwarding for Lab #2 and Lab #3 */  // NEW-LAB2

list <Op *> MEM_latch_list;
pipeline_latch *MEM_latch;  
pipeline_latch *EX_latch;
pipeline_latch *ID_latch;
pipeline_latch *FE_latch;
UINT64 ld_st_buffer[LD_ST_BUFFER_SIZE];   /* this structure is deprecated. do not use */ 
UINT64 next_pc; 

Cache *data_cache;  // NEW-LAB2 
//memory_c main_memory();   // NEW-LAB2 

/*******************************************************************/
/*  Print messages  */
/*******************************************************************/

void print_stats() {
  std::ofstream out((KNOB(KNOB_OUTPUT_FILE)->getValue()).c_str());
  /* Do not modify this function. This messages will be used for grading */ 
  out << "Total instruction: " << retired_instruction << endl; 
  out << "Total cycles: " << cycle_count << endl; 
  float ipc = (cycle_count ? ((float)retired_instruction/(float)cycle_count): 0 );
  out << "IPC: " << ipc << endl; 
  out << "Total I-cache miss: " << icache_miss_count << endl; 
  out << "Total D-cache hit: " << dcache_hit_count << endl; 
  out << "Total D-cache miss: " << dcache_miss_count << endl; 
  out << "Total L2-cache miss: " << l2_cache_miss_count << endl; 
  out << "Total data hazard: " << data_hazard_count << endl;
  out << "Total control hazard : " << control_hazard_count << endl; 
  out << "Total DRAM ROW BUFFER Hit: " << dram_row_buffer_hit_count << endl;  // NEW-LAB2
  out << "Total DRAM ROW BUFFER Miss: "<< dram_row_buffer_miss_count << endl;  // NEW-LAB2 
  out <<" Total Store-load forwarding: " << store_load_forwarding_count << endl;  // NEW-LAB2 

  out.close();
}

/*******************************************************************/
/*  Support Functions  */ 
/*******************************************************************/

bool get_op(Op *op)
{
  static UINT64 unique_count = 0; 
  Trace_op trace_op; 
  bool success = FALSE; 
  // read trace 
  // fill out op info 
  // return FALSE if the end of trace 
  success = (gzread(g_stream, &trace_op, sizeof(Trace_op)) >0 );
  if (KNOB(KNOB_PRINT_INST)->getValue()) dprint_trace(&trace_op); 

  /* copy trace structure to op */ 
  if (success) { 
    copy_trace_op(&trace_op, op); 

    op->inst_id  = unique_count++;
    op->valid    = TRUE; 
  }
  return success; 
}
/* return op execution cycle latency */ 

int get_op_latency (Op *op) 
{
  assert (op->opcode < NUM_OP_TYPE); 
  return op_latency[op->opcode];
}

/* Print out all the register values */ 
void dump_reg() {
  for (int ii = 0; ii < NUM_REG; ii++) {
    std::cout << cycle_count << ":register[" << ii  << "]: V:" << register_file[ii].valid << endl; 
  }
}

void print_pipeline() {
  std::cout << "--------------------------------------------" << endl; 
  std::cout <<"cycle count : " << dec << cycle_count << " retired_instruction : " << retired_instruction << endl; 
  std::cout << (int)cycle_count << " FE: " ;
  if (FE_latch->op_valid) {
    Op *op = FE_latch->op; 
    cout << (int)op->inst_id ;
  }
  else {
    cout <<"####";
  }
  std::cout << " ID: " ;
  if (ID_latch->op_valid) {
    Op *op = ID_latch->op; 
    cout << (int)op->inst_id ;
  }
  else {
    cout <<"####";
  }
  std::cout << " EX: " ;
  if (EX_latch->op_valid) {
    Op *op = EX_latch->op; 
    cout << (int)op->inst_id ;
  }
  else {
    cout <<"####";
  }


  std::cout << " MEM: " ;


	if( MEM_latch_list.size() == 0 ) {
	    cout <<"####";
	} 

	else {
  list<Op *>::iterator cii; 
  	for (cii= MEM_latch_list.begin() ; cii != MEM_latch_list.end(); cii++) {
   	Op* entry = (*cii);
      cout << (int)entry->inst_id;
		cout << " ";
	} }

/*
  if (MEM_latch->op_valid) {
    Op *op = MEM_latch->op; 
    cout << (int)op->inst_id ;
  }
  else {
    cout <<"####";
  }
*/
  cout << endl; 
  //  dump_reg();   
  std::cout << "--------------------------------------------" << endl; 
}

void print_heartbeat()
{
  static uint64_t last_cycle ;
  static uint64_t last_inst_count; 
  float temp_ipc = float(retired_instruction - last_inst_count) /(float)(cycle_count-last_cycle) ;
  float ipc = float(retired_instruction) /(float)(cycle_count) ;
  /* Do not modify this function. This messages will be used for grading */ 
  cout <<"**Heartbeat** cycle_count: " << cycle_count << " inst:" << retired_instruction << " IPC: " << temp_ipc << " Overall IPC: " << ipc << endl; 
  last_cycle = cycle_count;
  last_inst_count = retired_instruction; 
}
/*******************************************************************/
/*                                                                 */
/*******************************************************************/

bool run_a_cycle(memory_c *main_memory){   // please modify run_a_cycle function argument  /** NEW-LAB2 */ 

  for (;;) { 
    if (((KNOB(KNOB_MAX_SIM_COUNT)->getValue() && (cycle_count >= KNOB(KNOB_MAX_SIM_COUNT)->getValue())) || 
      (KNOB(KNOB_MAX_INST_COUNT)->getValue() && (retired_instruction >= KNOB(KNOB_MAX_INST_COUNT)->getValue())) ||  (sim_end_condition))) { 
        // please complete sim_end_condition 
        // finish the simulation 
        print_heartbeat(); 
        print_stats();
        return TRUE; 
    }
    cycle_count++; 
    if (!(cycle_count%5000)) {
      print_heartbeat(); 
    }

    main_memory->run_a_cycle();          // *NEW-LAB2 
    
    WB_stage(); 
    MEM_stage(main_memory);  // please modify MEM_stage function argument  /** NEW-LAB2 */ 
    EX_stage();
    ID_stage();
    FE_stage(); 
    if (KNOB(KNOB_PRINT_PIPE_FREQ)->getValue() && !(cycle_count%KNOB(KNOB_PRINT_PIPE_FREQ)->getValue())) print_pipeline();
  }
  return TRUE; 
}


/*
----------------------MY CODE
*/

void init_registers(void)
{
	int i = 0;
	for(i=0; i < NUM_REG; ++i) {
		register_file[i].valid = true;
	}

}

bool ID_stall = false;
bool cf_stall = false;
bool EX_busy = false;
bool MEM_busy = false;

bool moreInstructions = true;
/*
----------------------END OF MY CODE
*/
/*******************************************************************/
/* Complete the following fuctions.  */
/* You can add new data structures and also new elements to Op, Pipeline_latch data structure */ 
/*******************************************************************/

void init_structures(memory_c *main_memory) // please modify init_structures function argument  /** NEW-LAB2 */ 
{
  init_op_pool(); 
  init_op_latency();
  /* please initialize other data stucturs */ 
  /* you must complete the function */
  init_latches();
  init_registers();

	main_memory->init_mem();

	data_cache = (Cache *)malloc( sizeof(Cache) );
	int dcache_size = KNOB(KNOB_DCACHE_SIZE)->getValue();
	int block_size = 64;
	int associativity = KNOB(KNOB_DCACHE_WAY)->getValue();
	char *cache_name = "Data Cache";
	cache_init( data_cache, dcache_size, block_size, associativity, cache_name );

}

void WB_stage()
{
	printf( "MEM_busy: %d\n", MEM_busy );
	printf( "EX_Busy: %d\n", EX_busy );
	printf( "cf_stall: %d\n", cf_stall );
	printf( "ID_stall: %d\n", ID_stall );

	if( request_count == 0 && !moreInstructions && !EX_latch->op_valid && !ID_latch->op_valid && !FE_latch->op_valid ) {
		printf( "end condition\n" );
		sim_end_condition = true;
	}

	Op *op = NULL;
	while( MEM_latch_list.size() != 0 ) {
		op = MEM_latch_list.front();
		MEM_latch_list.pop_front();		

		register_file[ op->dst ].valid = true;
		if( op != NULL ) {
			printf( "op freed\n" );
			free_op(op);
		}
		++retired_instruction;
	  /* You MUST call free_op function here after an op is retired */ 
	  /* you must complete the function */

		DEBUG printf( "cf_type: %d\n", op->cf_type );
		if( op->cf_type )
			cf_stall = false;
	}

/*
	//Op *op = MEM_latch->op;
	Op *op = MEM_latch_list.front();

	DEBUG printf( "---WB------------\n" );
	if( op != NULL ) {
		DEBUG printf( "opcode: %u\n", op->opcode );
	}

	if( !moreInstructions && !EX_latch->op_valid && !ID_latch->op_valid && !FE_latch->op_valid ) {
		sim_end_condition = true;
	}


	//if( MEM_latch->op_valid == false ) {
	if( MEM_latch_list.size() == 0 ) {
		DEBUG printf( "op invalid\n" );
		return;  
	}
	MEM_latch_list.pop_front();

	register_file[ op->dst ].valid = true;
	if( op != NULL ) {
		printf( "op freed\n" );
		free_op(op);
	}
	++retired_instruction;
  // You MUST call free_op function here after an op is retired
  // you must complete the function

	DEBUG printf( "cf_type: %d\n", op->cf_type );
	if( op->cf_type )
		cf_stall = false;
*/
}

int MEM_cycle_count = 1;
void MEM_stage(memory_c *main_memory)  // please modify MEM_stage function argument  /** NEW-LAB2 */ 
{
	Op *op = EX_latch->op;

	DEBUG printf( "---MEM------------\n" );
	if( op != NULL )
		DEBUG printf( "opcode: %u\n", op->opcode );

	DEBUG printf( "op_valid: %d\n", EX_latch->op_valid );
	//op bubble
	if( EX_latch->op_valid == false ) {
		MEM_latch->op_valid = false;
		return;
	}

	//If OP uses memory simulate dcache latency

/*
	if( op->mem_type != NOT_MEM ) {
		if( MEM_cycle_count < KNOB(KNOB_DCACHE_LATENCY)->getValue() ) {
			++MEM_cycle_count;
			//send bubble forward during stall
			//MEM_latch->op_valid = false;
			//stall
			MEM_busy = true;
			return;
		}
		MEM_cycle_count = 1;
	}
*/

	if( op->mem_type != NOT_MEM ) {
		printf( "mem instruction\n" );
		if( MEM_cycle_count < KNOB(KNOB_DCACHE_LATENCY)->getValue() ) {
			++MEM_cycle_count;
			//send bubble forward during stall
			//MEM_latch->op_valid = false;
			//stall
			MEM_busy = true;
			return;
		}
		MEM_cycle_count = 1;
		MEM_busy = false;

		bool cache_hit = false;
		cache_hit = dcache_access( op->ld_vaddr );

		//Step 2:[2] cache miss
		if( cache_hit == false ) {

			printf( "cache miss\n" );
/*
			bool is_piggyback = false;
			//Step 2:[3] cache piggyback
			is_piggyback = main_memory->check_piggyback( op );
			//Step 2:[3] piggyback load store forwarding
			//this part should go in store_load_forwarding()
			if( is_piggyback && op->mem_type == MEM_LD ) {
				printf( "will piggyback\n" );
				m_mshr_entry_s * entry = main_memory->search_matching_mshr( op->ld_vaddr );
				printf( "entry valid: %d\n", entry->valid );
			}
			if( is_piggyback ) {
				//printf( "is_piggyback\n" );
				MEM_busy = false;
				//MEM_latch_list.push_back( op );
				return;
			}
*/

			//Step 2:[3] piggyback load store forwarding
			bool store_load_forward = main_memory->store_load_forwarding(op);
			if( store_load_forward ) {
				MEM_latch_list.push_back( op );
				MEM_busy = false;
				printf( "store load forward hit\n" );
				return;
			}

			//Step 2:[3] cache piggyback
			bool is_piggyback = main_memory->check_piggyback( op );
			if( is_piggyback ) {
				printf( "is_piggyback\n" );
				MEM_busy = false;
				return;
			}

			//Step 2:[4] cache miss, MSHR full
			if( main_memory->m_mshr.size() == main_memory->m_mshr_size ) {
				printf( "mshr stall\n" );
				printf( "actual size: %d max size: %d\n", main_memory->m_mshr.size(), main_memory->m_mshr_size );
				MEM_busy = true;
				return;
			}

			//Step 2:[4] cache miss, MSHR not full
			printf("insert_mshr inst_id: %d\n", op->inst_id);

			main_memory->insert_mshr( op );

			MEM_busy = false;
			return;

		}
		//Step 2:[1] cache hit
		else {
			printf( "cache hit\n" );
			MEM_latch_list.push_back( op );
			MEM_busy = false;
			return;
		}

	}



	/*
	DEBUG printf( "cf_type: %d\n", op->cf_type );
	if( op->cf_type )
		cf_stall = false;
	*/

	MEM_latch_list.push_back( op );

 	//MEM_latch->op = op; 
	//MEM_latch->op_valid = true;
  /* you must complete the function */

}


int EX_cycle_count = 1;
void EX_stage() 
{
	Op *op = ID_latch->op;

	DEBUG printf( "---ALU------------\n" );
	if( op != NULL ) {
		DEBUG printf( "opcode: %u\n", op->opcode );
	}

	//MEM STALL
	if( MEM_busy ) {
		return;
	}

	if( ID_latch->op_valid == false ) {
		EX_latch->op_valid = false;
		return;
	}
	DEBUG printf("opcode valid\n");

	if( EX_cycle_count < get_op_latency(op) ) {
		++EX_cycle_count;
		EX_busy = true;
		EX_latch->op_valid = false;
		DEBUG printf("latency stall\n");
	}
	else {
		DEBUG printf("execution complete\n");
		EX_cycle_count = 1;
		EX_busy = false;

	   EX_latch->op = op;
		EX_latch->op_valid = true;
	}
  /* you must complete the function */
}

void ID_stage()
{
	Op *op = FE_latch->op;

	DEBUG printf( "---decoder------------\n" );
	if( op != NULL )
	DEBUG printf( "opcode: %u\n", op->opcode );
	DEBUG printf( "ID_stall: %d\n", ID_stall );
	DEBUG printf( "cf_stall: %d\n", cf_stall );
	//Stall if EX is busy
	//ID4
	//If EX busy due to get_op_latency() stall ID and FE
	if( EX_busy == true || MEM_busy == true ) {
		ID_stall = true;
		DEBUG printf("latency stall\n");
		//ID_latch->op_valid = false;
		return;
	}
	//propagate bubble if necessary
	if( FE_latch->op_valid == false ) {
		ID_latch->op_valid = false;
		ID_stall = false;
		return;
	}
	DEBUG printf("opcode valid\n");

	//ID1
	//If src register not valid stall ID and FE
	bool src_valid = true;
	if( op->num_src != -1 ) {
		int i=0;
		for(i=0; i < op->num_src; ++i) {
			if( register_file[op->src[i]].valid == false )
				src_valid = false;
		}
	}
	if( src_valid == false ) {
		ID_stall = true;
		ID_latch->op_valid = false;
		++data_hazard_count;
		DEBUG printf("data hazard count: %d\n", data_hazard_count);
		//test
		if( op->cf_type ) ++control_hazard_count;

		return;
	}

	//ID3
	//If control flow instruction, stall FE until target address forwarded from MEM_stage()
	if( op->cf_type ) {
		cf_stall = true;
		++control_hazard_count;
		DEBUG printf("control hazard count: %d\n", control_hazard_count);
	}
	//ID2
	//There are no stalls
	ID_stall = false;
	//If DST register used, set the register to invalid until WB_stage()
	if( op->dst != -1 )
		register_file[ op->dst ].valid = false;
	
	ID_latch->op = op;
	ID_latch->op_valid = true;
		
	
  //printf( "%ull\n", op->inst_id);
  /* you must complete the function */
}


void FE_stage()
{
  	/* only part of FE_stage function is implemented */ 
	/* please complete the rest of FE_stage function */ 

	DEBUG printf( "testing fetch-------------------------\n" );
	if( cf_stall == false && ID_stall == false ) {
		Op *op = get_free_op();
 		moreInstructions = get_op(op); 

		DEBUG printf( "opcode: %u\n", op->opcode );


	   FE_latch->op = op;
		FE_latch->op_valid = moreInstructions;
	   //next_pc = pc + op->inst_size;  // you need this code for building a branch predictor 
	}
	//Send bubble
	else if( cf_stall ) {
		DEBUG printf("stalled\n");
		FE_latch->op_valid = false;
	}

	DEBUG print_pipe();
}


void print_pipe()
{
	printf("[ cf_stall: %d ]-->", cf_stall);

	printf("{ inst_id: %d op_dst: %d }", FE_latch->op_valid ? FE_latch->op->inst_id : -1,
	FE_latch->op_valid ? FE_latch->op->dst : -99);
	printf("[ id_stall: %d ]-->", ID_stall);

	printf("{ inst_id: %d op_dst: %d }", ID_latch->op_valid ? ID_latch->op->inst_id : -1,
	ID_latch->op_valid ? ID_latch->op->dst : -99);
	printf("[ EX_busy: %d ]-->", EX_busy);

	printf("{ inst_id: %d }-->", EX_latch->op_valid ? EX_latch->op->inst_id : -1);


	printf("{ inst_id: %d }-->", MEM_latch->op_valid ? MEM_latch->op->inst_id : -1);

	//printf("[ inst_id: %d ]-->", ID_latch->op->inst_id );
	//printf("[ inst_id: %d ]-->", EX_latch->op->inst_id );
}



void  init_latches()
{
  MEM_latch = new pipeline_latch();
  EX_latch = new pipeline_latch();
  ID_latch = new pipeline_latch();
  FE_latch = new pipeline_latch();
  //MEM_latch_list = new list<Op *>;
	MEM_latch_list.clear();

  MEM_latch->op = NULL;
  EX_latch->op = NULL;
  ID_latch->op = NULL;
  FE_latch->op = NULL;

  /* you must set valid value correctly  */ 
  MEM_latch->op_valid = false;
  EX_latch->op_valid = false;
  ID_latch->op_valid = false;
  FE_latch->op_valid = false;

}

bool icache_access(ADDRINT addr) {   /** please change uint32_t to ADDRINT NEW-LAB2 */ 

  /* For Lab #1, you assume that all I-cache hit */     
  bool hit = FALSE; 
  if (KNOB(KNOB_PERFECT_ICACHE)->getValue()) hit = TRUE; 
  return hit; 
}


bool dcache_access(ADDRINT addr) { /** please change uint32_t to ADDRINT NEW-LAB2 */ 
  /* For Lab #1, you assume that all D-cache hit */     
  /* For Lab #2, you need to connect cache here */   // NEW-LAB2 
  bool hit = FALSE;
  if (KNOB(KNOB_PERFECT_DCACHE)->getValue()) hit = TRUE; 
  
	else {
		hit = cache_access(data_cache, addr);	
	}
  return hit; 
}



// NEW-LAB2 
void dcache_insert(ADDRINT addr)  // NEW-LAB2 
{                                 // NEW-LAB2 
  /* dcache insert function */   // NEW-LAB2 
  cache_insert(data_cache, addr) ;   // NEW-LAB2 
 
}                                       // NEW-LAB2 

void broadcast_rdy_op(Op* op)             // NEW-LAB2 
{                                          // NEW-LAB2 
  /* you must complete the function */     // NEW-LAB2 
  // mem ops are done.  move the op into WB stage   // NEW-LAB2 
	MEM_latch_list.push_back(op);
	if( op->mem_type == MEM_LD )
		dcache_insert( op->ld_vaddr );
	else if( op->mem_type == MEM_ST )
		dcache_insert( op->st_vaddr );
	printf( "broadcasting cycle_count: %d, inst_id: %d\n", cycle_count, op->inst_id );
}      // NEW-LAB2 



/* utility functions that you might want to implement */     // NEW-LAB2 
int get_dram_row_id(ADDRINT addr)    // NEW-LAB2 
{  // NEW-LAB2 
 // NEW-LAB2 
/* utility functions that you might want to implement */     // NEW-LAB2 
/* if you want to use it, you should find the right math! */     // NEW-LAB2 
/* pleaes carefull with that DRAM_PAGE_SIZE UNIT !!! */     // NEW-LAB2 
  // addr >> 6;   // NEW-LAB2 
  return 2;   // NEW-LAB2 
}  // NEW-LAB2 

int get_dram_bank_id(ADDRINT addr)  // NEW-LAB2 
{  // NEW-LAB2 
 // NEW-LAB2 
/* utility functions that you might want to implement */     // NEW-LAB2 
/* if you want to use it, you should find the right math! */     // NEW-LAB2 

  // (addr >> 6);   // NEW-LAB2 
  return 1;   // NEW-LAB2 
}  // NEW-LAB2 



void memtest_helper(memory_c *main_memory) {
	bool MEM_busy = false;

	Op *op = new Op;
	op->mem_type = MEM_LD;
	op->ld_vaddr = 2000;
	op->mem_read_size = 20;

	printf( "MEM_cycle_count: %d dcache latency: %d\n", MEM_cycle_count, KNOB(KNOB_DCACHE_LATENCY)->getValue() );

	if( op->mem_type != NOT_MEM ) {
		printf( "mem instruction\n" );
		if( MEM_cycle_count < KNOB(KNOB_DCACHE_LATENCY)->getValue() ) {
			++MEM_cycle_count;
			//send bubble forward during stall
			//MEM_latch->op_valid = false;
			//stall
			MEM_busy = true;
			return;
		}
		MEM_cycle_count = 1;
		MEM_busy = false;

		bool cache_hit = false;
		cache_hit = dcache_access( op->ld_vaddr );

		//Step 2:[2] cache miss
		if( cache_hit == false ) {

			printf( "cache miss\n" );


/*
			//this part should go in store_load_forwarding()
			if( op->mem_type == MEM_LD ) {
				printf( "will piggyback\n" );
				m_mshr_entry_s * entry = main_memory->search_matching_mshr( op->ld_vaddr );
				if( entry != NULL ) {
  					list<Op *>::const_iterator cii;
	  				for (cii= entry->req_ops.begin() ; cii != entry->req_ops.end(); cii++) {
						Op *tempOp = (*cii);
						printf("tempOp->inst_id: %d\n", tempOp->inst_id);
						if( tempOp->mem_type == MEM_ST && 
							 op->ld_vaddr >= tempOp->st_vaddr && 
							 tempOp->mem_write_size +tempOp->st_vaddr >= op->mem_read_size + op->ld_vaddr ) {
							printf("store load forward successful\n");
						}
					}
				}
				printf( "entry valid: %d\n", entry->valid );
			}
*/
			//Step 2:[3] piggyback load store forwarding
			bool store_load_forward = main_memory->store_load_forwarding(op);
			if( store_load_forward ) {
				MEM_latch_list.push_back( op );
				MEM_busy = false;
				printf( "store load forward hit\n" );
				return;
			}

			//Step 2:[3] cache piggyback
			bool is_piggyback = main_memory->check_piggyback( op );
			if( is_piggyback ) {
				printf( "is_piggyback\n" );
				MEM_busy = false;
				return;
			}

			//Step 2:[4] cache miss, MSHR full
			if( main_memory->m_mshr.size() == main_memory->m_mshr_size ) {
				printf( "mshr stall\n" );
				printf( "actual size: %d max size: %d\n", main_memory->m_mshr.size(), main_memory->m_mshr_size );
				MEM_busy = true;
				return;
			}

			//Step 2:[4] cache miss, MSHR not full
			printf("insert_mshr inst_id: %d\n", op->inst_id);

			main_memory->insert_mshr( op );

			MEM_busy = false;
			return;

		}
		//Step 2:[1] cache hit
		else {
			printf( "cache hit\n" );
			MEM_latch_list.push_back( op );
			MEM_busy = false;
			return;
		}

	}

}

void memory_test(memory_c *mem) {
	memory_c *main_memory = mem;
	//main_memory.dprint_queues();
	mem->init_mem();
	data_cache = (Cache *)malloc( sizeof(Cache) );
	//cache_init(Cache *cache,unsigned int cache_size, int block_size,unsigned int assoc, const char *s)


	int dcache_size = KNOB(KNOB_DCACHE_SIZE)->getValue();
	int block_size = 64;
	int associativity = KNOB(KNOB_DCACHE_WAY)->getValue();
	char *cache_name = "Data Cache";
	cache_init( data_cache, dcache_size, block_size, associativity, cache_name );

	//cache_init( data_cache, 1, 64, 1, "aCache" );
	//cache_insert( data_cache, 4 );

	printf( "-----memory-test-----------\n" );
	printf( "%s\n", data_cache->name );

/*
	ADDRINT x = 4000;
	//printf( "cache_access: %d\n", cache_access(Cache *cache, ADDRINT addr) );
	printf( "dcache_access: %d\n", dcache_access( x ) );
*/

/*
	Op *op = new Op;
	op->mem_type = MEM_LD;
	op->ld_vaddr = 2000;
*/

/*
	
	//printf( "search_match_mshr: %d\n", mem->search_matching_mshr( op_mshr->st_vaddr ) );
	//printf( "check_piggyback: %d\n", main_memory->check_piggyback( op ) );
	printf( "mshr size: %d\n", mem->m_mshr_size );
	printf( "mshr in use: %d\n", mem->m_mshr.size() );
  	//MEM_latch_list = new list<Op *>;
	MEM_latch_list.clear();
	printf( "MEM_latch_list.size(): %d\n", MEM_latch_list.size() );
*/

	//test op for store load forwarding

	Op *op_mshr = new Op;
	op_mshr->mem_type = MEM_ST;
	op_mshr->inst_id = 66;
	op_mshr->st_vaddr = 2000;
	op_mshr->mem_write_size = 10;
	mem->insert_mshr( op_mshr );

memtest_helper( main_memory );

int i = 0;
for(i = 0; i < 205; ++i) {
	++cycle_count;
	mem->run_a_cycle();
	printf(" run a cycle\n" );
}

/*
	//If OP uses memory simulate dcache latency
	if( op->mem_type != NOT_MEM ) {
		if( MEM_cycle_count < KNOB(KNOB_DCACHE_LATENCY)->getValue() ) {
			++MEM_cycle_count;
			//send bubble forward during stall
			//MEM_latch->op_valid = false;
			//stall
			MEM_busy = true;
			return;
		}
		MEM_cycle_count = 1;


		cache_hit = dcache_access( op->ld_vaddr );
		//Step 2:[2] cache miss
		if( cache_hit == false ) {
			printf( "cache miss\n" );
			bool is_piggyback = false;
			//Step 2:[3] cache piggyback
			is_piggyback = main_memory->check_piggyback( op );
			//Step 2:[3] piggyback load store forwarding
			//this part should go in store_load_forwarding()
			if( is_piggyback && op->mem_type == MEM_LD ) {
				printf( "will piggyback\n" );
				m_mshr_entry_s * entry = main_memory->search_matching_mshr( op->ld_vaddr );
				printf( "entry valid: %d\n", entry->valid );
			}
			if( is_piggyback ) {
				MEM_BUSY = false;
				MEM_latch_list.push_back( op );
				return;
			}

			//Step 2:[4] cache miss, MSHR full
			if( main_memory->m_mshr.size() == main_memory->m_mshr_size ) {
				printf( "mshr stall\n" );
				MEM_BUSY = true;
				return;
			}

			//Step 2:[4] cache miss, MSHR not full
			printf("insert_mshr\n");
			main_memory->insert_mshr( op );
			MEM_busy = false;
			return;
		}
		//Step 2:[1] cache hit
		else {
			MEM_BUSY = false;
			return;
		}
	}
*/
/*
	Op *LoadOp1 = new Op;
	LoadOp1->mem_type = MEM_LD;
	LoadOp1->ld_vaddr = 2047;

	Op *LoadOp2 = new Op;
	LoadOp2->mem_type = MEM_LD;
	//LoadOp2->ld_vaddr = 4096;
	LoadOp2->ld_vaddr = 2046;

	mem->insert_mshr( LoadOp2 );
	mem->insert_mshr( LoadOp1 );
*/
/*	
   int i=0;
	while( false != mem->insert_mshr( LoadOp1 ) )
		++i;
	printf( "mshr size: %d\n", i);
*/


/*
	mem->run_a_cycle();
	//mem->dprint_queues();
++cycle_count;
	mem->run_a_cycle();
	//mem->dprint_queues();
++cycle_count;
	mem->run_a_cycle();
	//mem->dprint_queues();
*/

/*
int i = 0;
for(i = 0; i < 205; ++i) {
	mem->run_a_cycle();
	++cycle_count;
}
*/

//++cycle_count;
//	mem->run_a_cycle();
//	mem->dprint_queues();
//	mem->run_a_cycle();
/*
*/
	//mem->run_a_cycle();

/*
int i = 0;
for(i = 0; i < 104; ++i) {
	++cycle_count;
	mem->run_a_cycle();
}
*/

	//mem->dprint_dram_banks();


/*
	list<m_mshr_entry_s *>::iterator cii; 
	for (cii= mem->m_mshr.begin() ; cii != mem->m_mshr.end(); cii++) {
		m_mshr_entry_s* entry = (*cii);
		mem_req_s *req = entry->m_mem_req;
		printf( "%d\n", req->m_state );
		//mem->dram_in_queue.push_back(req);	
	}


	printf( "dram_in_queue\n" );
  	list<mem_req_s *>::iterator dii;   
	for(dii = mem->dram_in_queue.begin(); dii != mem->dram_in_queue.end(); dii++) {
		mem_req_s *req = (*dii);

		printf( "%d\n", req->m_state );
	}
*/
	//mem->run_a_cycle();

	//mem->run_a_cycle();
}    



