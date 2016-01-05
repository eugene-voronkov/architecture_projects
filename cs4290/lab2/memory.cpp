#include "common.h"
#include "sim.h"
#include "trace.h" 
#include "memory.h" 
#include "knob.h"
#include "all_knobs.h"

#include <stdlib.h>




#ifdef DEBUG_ON
 #define DEBUG if(1) 
#else
 #define DEBUG if(0) 
#endif

#define RUN_MEM

#ifdef RUN_MEM
 #define MEMTEST_OFF if(0)
 #define MEMTEST_ON if(1)
#else
 #define MEMTEST_OFF if(1)
 #define MEMTEST_ON if(0) 
#endif


int request_count = 0;
/*******************************************************************/
/* Memory Related frame */ 
/*******************************************************************/
/* You do not need to change the code in this part                 */ 

void memory_c::dprint_queues(void) 
{
  // traverse the queue and print out all the values inside the queue 

  /* print out mshr entries */ 
  
  list<m_mshr_entry_s *>::const_iterator cii; 
  
  /* do not modify print MSHR print statement here. These will be used for grading */ 

/*
  cout <<"** cycle_count: " << cycle_count << endl; 
  cout <<"***MSHR entry print outs: mshr_size: " <<  m_mshr.size() << endl; 


  for (cii= m_mshr.begin() ; cii != m_mshr.end(); cii++) {
    
    int jj = 0; 
    cout << "mshr_entry_num: " << jj;
    jj++; 

    m_mshr_entry_s* entry = (*cii);
    
    if (entry->valid) {
      mem_req_s *req = entry->m_mem_req; 
      
      cout <<" mem_req_id: " << req->m_id; 
      cout <<" mem_addr: " << req->m_addr; 
      cout <<" size: " << (int)req->m_size; 
      cout <<" insert_time: " << entry->insert_time;
      cout <<" rdy_cycle: " << req->m_rdy_cycle; 
      cout <<" dirty: " << req->m_dirty; 
      cout <<" done: " << req->m_done; 
      cout <<" state: " << req->m_state; 
      cout <<" type:  " << req->m_type; 
      cout <<" ops_size: " << entry->req_ops.size(); 
      
      if (entry->req_ops.size()) {
	list<Op *>::const_iterator cii; 
	int kk = 0; 
	for (cii = entry->req_ops.begin() ; cii != entry->req_ops.end(); cii++) {
	  
	  Op *m_op = (*cii);
	  
	  if(m_op->mem_type == MEM_LD) {
	    printf("op[%d]:LD id: %lu ", kk, (uint64_t)m_op->inst_id);
	  }
	  else 
	    printf("op[%d]:ST id: %lu", kk, (uint64_t)m_op->inst_id);
	  	  
	}
	kk++;

      }
     
      cout << endl; 
    }
  }
  
  // print queues 

  
  cout <<"***DRAM_IN_QUEUE entry print outs ****" << endl; 
*/  
  list<mem_req_s *>::const_iterator cqii; 
/*
  for (cqii = dram_in_queue.begin() ; cqii != dram_in_queue.end(); cqii++) {
    mem_req_s *req = *cqii; 
    
    cout << " req_id: " << req->m_id; 
    cout << " addr: " << req->m_addr; 
    cout << " rdy_cycle: " << req->m_rdy_cycle; 
    cout <<" state: " << req->m_state; 
    cout <<" type:  " << req->m_type; 
    cout << " ||| " ; 
  }
  
  cout << endl; 
  // end printing dram in queue 
  
*/
  // print dram scheduler queues 
  
  list<mem_req_s *>::const_iterator clii; 
  
  cout <<"***DRAM_SCH_QUEUE entry print outs ****" << endl; 

  for (int bb = 0; bb < m_dram_bank_num; bb++) { 
    cout <<"***DRAM_SCH_QUEUE BANK[" << bb << "]" << endl; 
    
    for (clii = dram_bank_sch_queue[bb].begin(); 
	 clii != dram_bank_sch_queue[bb].end(); 
	 clii++) { 
      
      mem_req_s *req = *clii; 
      
      cout << " req_id: " << req->m_id; 
      cout << " addr: " << req->m_addr; 
      cout << " rdy_cycle: " << req->m_rdy_cycle; 
      cout <<" state: " << req->m_state; 
      cout <<" type:  " << req->m_type; 
      cout << " ||| " ; 
    
    }
    cout << endl; 
  }
  cout << endl; 
  // ending print dram scheduler 

  
  // print dram out queue 
  
  cout <<"***DRAM_OUT_QUEUE entry print outs ****" << endl; 
  for (cqii= dram_out_queue.begin() ; cqii != dram_out_queue.end(); cqii++) {
    mem_req_s *req = *cqii; 
    
    cout << " req_id: " << req->m_id; 
    cout << " addr: " << req->m_addr; 
    cout << " rdy_cycle: " << req->m_rdy_cycle; 
    cout <<" state: " << req->m_state; 
    cout <<" type:  " << req->m_type; 
    cout << " ||| " ; 
      
  }
  
  cout << endl; 
  
  
}

/*******************************************************************/
/* print dram status. debugging help function 
/*******************************************************************/

void memory_c::dprint_dram_banks()
{
  
  cout << " DRAM_BANK_STATUS cycle_count "<< cycle_count << endl; 

  for (int ii = 0; ii <m_dram_bank_num; ii++) {
    printf("bank_num[%d]: row_id:%d rdy_cycle:%lu \n",
	   ii, dram_bank_open_row[ii], (uint64_t)dram_bank_rdy_cycle[ii]); 
  }
  
}


/*******************************************************************/
/* Initialize the memory related data structures                   */
/*******************************************************************/

void memory_c::init_mem()
{
  /* For Lab #2, you do not need to modify this code */

  /* you can add other code here if you want */ 
  

  /* init mshr */ 
  m_mshr_size = KNOB(KNOB_MSHR_SIZE)->getValue(); 
  m_dram_bank_num = KNOB(KNOB_DRAM_BANK_NUM)->getValue(); 
  m_block_size = KNOB(KNOB_BLOCK_SIZE)->getValue();

  
  for (int ii = 0 ; ii < m_mshr_size; ii++) 
  {

    m_mshr_entry_s* entry = new m_mshr_entry_s; 
    entry->m_mem_req = new mem_req_s;  // create a memory rquest data structure here 
    
    entry->valid = false; 
    entry->insert_time = 0; 
    m_mshr_free_list.push_back(entry); 
  }
  


  /* init DRAM scheduler queues */ 

  dram_in_queue.clear();
  dram_out_queue.clear();
  
  dram_bank_sch_queue = new list<mem_req_s*>[m_dram_bank_num]; 
  dram_bank_open_row = new int[m_dram_bank_num];

  dram_bank_rdy_cycle = new int[m_dram_bank_num]; 
  
  	for(int ii=0;ii<m_dram_bank_num;ii++)
	 	dram_bank_open_row[ii] = -1;
}

/*******************************************************************/
/* free MSHR entry (init the corresponding mshr entry  */ 
/*******************************************************************/

void memory_c::free_mshr_entry(m_mshr_entry_s *entry)
{
  entry->valid = false; 
  entry->insert_time = 0; 
  m_mshr_free_list.push_back(entry); 

}

/*******************************************************************/
/* This function is called every cycle                             */ 
/*******************************************************************/

void memory_c::run_a_cycle() 
{
	DEBUG printf( "runacycle\n" );
  
  if (KNOB(KNOB_PRINT_MEM_DEBUG)->getValue()) {
    dprint_queues();
    dprint_dram_banks();
  }
  
  /* This function is called from run_a_cycle() every cycle */ 
  /* You do not add new code here */ 
  /* insert D-cache/I-cache (D-cache for only Lab #2) and wakes up instructions */ 
  fill_queue(); 

cout << "test in run a cycle";
//dprint_queues();

  /* move memory requests from dram to cache and MSHR*/ /* out queue */ 
  send_bus_out_queue(); 

//dprint_queues();
  /* memory requests are scheduled */ 
  dram_schedule(); 

//dprint_queues();

  /* memory requests are moved from bus_queue to DRAM scheduler */
  push_dram_sch_queue();
  
//dprint_queues();

  /* new memory requests send from MSRH to in_bus_queue */ 
  send_bus_in_queue(); 

if(cycle_count > 5165) dprint_queues();
}

/*******************************************************************/
/* get a new mshr entry 
/*******************************************************************/

m_mshr_entry_s* memory_c::allocate_new_mshr_entry(void)
{
  
  if (m_mshr_free_list.empty())
    return NULL; 
  
  m_mshr_entry_s* entry = m_mshr_free_list.back(); 
  m_mshr_free_list.pop_back(); 
  m_mshr.push_back(entry); 
  
  return entry; 
  
 }






/* insert memory request into the MSHR, if there is no space return false */ 
/*******************************************************************/
/* memory_c::insert_mshr
/*******************************************************************/
bool memory_c::insert_mshr(Op *mem_op)
{
	++request_count;

	DEBUG printf( "insert_mshr: %d\n", mem_op->inst_id );
  bool insert=false; 
 
  //  step 1. create a new memory request 
  m_mshr_entry_s * entry = allocate_new_mshr_entry();
 
  //  step 2. no free entry means no space return ; 
  if (!entry) return false; // cannot insert a request into the mshr; 

  // insert op to into the mshr 
  entry->req_ops.push_back(mem_op);

    // step 3. initialize the memory request here 
  mem_req_s *t_mem_req = entry->m_mem_req; 
  
  if (mem_op->mem_type == MEM_LD ) { 
    t_mem_req->m_type = MRT_DFETCH; 
    t_mem_req->m_addr = mem_op->ld_vaddr; 
    t_mem_req->m_size = mem_op->mem_read_size;
  }
  else {
    t_mem_req->m_type = MRT_DSTORE; 
    t_mem_req->m_addr = mem_op->st_vaddr; 
    t_mem_req->m_size = mem_op->mem_write_size;
  }
    t_mem_req->m_rdy_cycle = 0; 
    t_mem_req->m_dirty = false; 
    t_mem_req->m_done = false; 
    t_mem_req->m_id = ++m_unique_m_count; 
    t_mem_req->m_state = MEM_NEW; 
    
    entry->valid  = true; 
    entry->insert_time = cycle_count; 
    
    return true; 
}


/*******************************************************************/
/* searching for matching mshr entry using memory address 
/*******************************************************************/


m_mshr_entry_s * memory_c::search_matching_mshr(ADDRINT addr) {
  
  list<m_mshr_entry_s *>::const_iterator cii; 
  
  for (cii= m_mshr.begin() ; cii != m_mshr.end(); cii++) {
    
    m_mshr_entry_s* entry = (*cii);
    if (entry->valid) {
      mem_req_s *req = entry->m_mem_req; 
      if (req) {
	if ((req->m_addr)/m_block_size == (addr/m_block_size)) 
	  return entry; 
      }
    }
  }
  return NULL;
}

/*******************************************************************/
/* searching for matching mshr entry using memory address and return the iterator 
/*******************************************************************/

list<m_mshr_entry_s*>::iterator memory_c::search_matching_mshr_itr(ADDRINT addr) {
  
  list<m_mshr_entry_s *>::iterator cii; 
  
  for (cii= m_mshr.begin() ; cii != m_mshr.end(); cii++) {
    
    m_mshr_entry_s* entry = (*cii);
    if (entry->valid) {
      mem_req_s *req = entry->m_mem_req; 
      if (req) {
	if ((req->m_addr)/m_block_size == (addr/m_block_size)) 
	  return cii; 
      }
    }
  }
  return cii;
}



/*******************************************************************/
/*  search MSHR entries and find a matching MSHR entry and piggyback 
/*******************************************************************/

bool memory_c::check_piggyback(Op *mem_op)
{
  bool match = false; 
  ADDRINT addr;
  if (mem_op->mem_type == MEM_LD) 
    addr = mem_op->ld_vaddr;
  else 
    addr = mem_op->st_vaddr; 
  
  m_mshr_entry_s *entry = search_matching_mshr(addr);
  
  if(!entry)
  {
    return false;
  } else 
  {
    entry->req_ops.push_back(mem_op); 
    return true;
  }
}



/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/*****    COMPLETE the following functions               ***********/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/




/*******************************************************************/
/* send a request from mshr into in_queue 
/*******************************************************************/

void memory_c::send_bus_in_queue() {
  list<m_mshr_entry_s *>::iterator cii; 
  
	//printf("send_bus_in_queue()\n");
	bool stageComplete = false;
  for (cii= m_mshr.begin() ; cii != m_mshr.end() && !stageComplete; cii++) {
    m_mshr_entry_s* entry = (*cii);
	 mem_req_s *req = entry->m_mem_req;

	if( req->m_state == MEM_NEW ) {
		dram_in_queue.push_back(req);
		req->m_state = MEM_DRAM_IN;
		stageComplete = true;
	}
	//printf( "%d\n", req->m_state );
  }  
   /* For Lab #2, you need to fill out this function */ 
  
  // *END**** This is for the TA
} 




/*******************************************************************/
/*  push_dram_sch_queue()
/*******************************************************************/

void memory_c::push_dram_sch_queue()
{

  list<mem_req_s *>::iterator cii;
	bool bankFilled[ m_dram_bank_num ];
	int i = 0;
	for(i = 0; i < m_dram_bank_num; ++i) {
		bankFilled[i] = false;
	}

	for(cii = dram_in_queue.begin(); cii != dram_in_queue.end(); ) {
		mem_req_s *req = (*cii);
		ADDRINT address = req->m_addr;
		ADDRINT page_size = KNOB(KNOB_DRAM_PAGE_SIZE)->getValue();
		ADDRINT bank_id = (address/(page_size*1024))%(m_dram_bank_num);

		if( !bankFilled[bank_id] ) {
			dram_bank_sch_queue[bank_id].push_back(req);
			//req->m_state = MEM_DRAM_SCH;
			bankFilled[bank_id] = true;
			cii = dram_in_queue.erase(cii);
		}
		else {
			cii++;
		}
		//int bank_id = (req->m_addr/(KNOB(KNOB_DRAM_PAGE_SIZE)->getValue()*1024))%(m_dram_bank_num);

		DEBUG printf("push_dram_sch_queue() band_id: %d address: %d page_size: %d\n", bank_id, address, page_size );


	}
  /* For Lab #2, you need to fill out this function */ 
   
}


/*******************************************************************/
/* search from dram_schedule queue and scheule a request 
/*******************************************************************/

 void memory_c::dram_schedule() {
	DEBUG printf( "-------------------------------dram_schedule debug\n" );
	DEBUG printf( "cycle_count: %d\n", cycle_count );
	//printf( "KNOB_MEM_LATENCY_ROW_HIT: %d\n", (long unsigned int)KNOB(KNOB_MEM_LATENCY_ROW_HIT)->getValue() );
	//printf( "KNOB_MEM_LATENCY_ROW_MISS: %d\n", (long unsigned int)KNOB(KNOB_MEM_LATENCY_ROW_MISS)->getValue() );

	int bank_id=0;
	for(bank_id=0; bank_id < m_dram_bank_num; ++bank_id) {
		DEBUG printf( "outer loop dram_bank_rdy_cycle[ %d ]: %d\n", bank_id, dram_bank_rdy_cycle[ bank_id ] );
		//check the corresponding DRAM bank and see whether the DRAM bank is available
		if (dram_bank_rdy_cycle[ bank_id ] < cycle_count) {
			//printf("inner loop entered\n" );
			DEBUG printf("cii begin: %d cii end: %d\n", dram_bank_sch_queue[bank_id].begin(), dram_bank_sch_queue[bank_id].end() );
			bool found_unscheduled = false;
			list<mem_req_s *>::iterator cii;
			//dprint_queues();
		   //for (cii = dram_bank_sch_queue[bank_id].begin(); cii != dram_bank_sch_queue[bank_id].end(); cii++) {
			for(cii = dram_bank_sch_queue[bank_id].begin(); !found_unscheduled && cii != dram_bank_sch_queue[bank_id].end(); cii++) {
				mem_req_s *req = (*cii);
				//if( req->m_state == MEM_DRAM_SCH ) {
				DEBUG printf("m_state: %d\n", req->m_state );
				if( req->m_state == MEM_DRAM_IN ) {
				//if( req->m_state == MEM_DRAM_IN ) {
					DEBUG printf( "-----------------------mem request scheduled\n" );
					DEBUG printf( "dram_bank_sch_queue[ %d ]\n", bank_id );
					req->m_state = MEM_DRAM_SCH;
					found_unscheduled = true;

					ADDRINT address = req->m_addr;
					ADDRINT page_size = KNOB(KNOB_DRAM_PAGE_SIZE)->getValue();
					ADDRINT row_id = address/(page_size*1024);
					//If the memory request has the same row id
					if( dram_bank_open_row[ bank_id ] == row_id ) {
						req->m_rdy_cycle = cycle_count + (long unsigned int)KNOB(KNOB_MEM_LATENCY_ROW_HIT)->getValue();
						dram_bank_rdy_cycle[ bank_id ] = req->m_rdy_cycle;
					}
					//If the row buffer number of the corresponding dram bank is different from the row id
					else {
						req->m_rdy_cycle = cycle_count + (long unsigned int)KNOB(KNOB_MEM_LATENCY_ROW_MISS)->getValue();
						 dram_bank_open_row[ bank_id ] = row_id;
						 dram_bank_rdy_cycle[ bank_id ] = req->m_rdy_cycle;
					}
				}
			}
		}
	}
   /* For Lab #2, you need to fill out this function */ 
   
}




/*******************************************************************/
/* send bus_out_queue 
/*******************************************************************/

void memory_c::send_bus_out_queue() 
{

   /* For Lab #2, you need to fill out this function */ 

	int bank_id=0;
	for(bank_id=0; bank_id < m_dram_bank_num; ++bank_id) {
		if (dram_bank_rdy_cycle[ bank_id ] < cycle_count && dram_bank_sch_queue[ bank_id ].size() != 0) {
			mem_req_s *req = dram_bank_sch_queue[bank_id].front();
			if( req->m_state == MEM_DRAM_SCH && dram_bank_rdy_cycle[ bank_id ] == req->m_rdy_cycle ) {
				dram_out_queue.push_back( req );
				dram_bank_sch_queue[bank_id].pop_front();
				//dram_bank_sch_queue[bank_id].erase( dram_bank_sch_queue[bank_id].begin() );
				req->m_state = MEM_DRAM_OUT;
			}
		}
	}
  

}


/*******************************************************************/
/*  fill_queue 
/*******************************************************************/

void memory_c::fill_queue() 
{

   /* For Lab #2, you need to fill out this function */ 
  /* CAUTION!: This function is not completed. Please complete this function */ 
  
  if (dram_out_queue.empty()) return; 
  
    mem_req_s *req = dram_out_queue.front(); 
    dram_out_queue.pop_front(); 
    
    /* search for matching mshr  entry */ 
    m_mshr_entry_s *entry = search_matching_mshr(req->m_addr); 
    

    while(entry->req_ops.size()) {
      Op *w_op = entry->req_ops.front();
      broadcast_rdy_op(w_op); 
      entry->req_ops.pop_front(); 
    }
    
    /* The following code will free mshr entry */ 
    list<m_mshr_entry_s *>::iterator mii = search_matching_mshr_itr(req->m_addr); 
    m_mshr.erase(mii); 
  
  free_mshr_entry(entry); 

  --request_count;
}





/*******************************************************************/
/*  store-load forwarind features, cache addresses, cache load/store types 
/*******************************************************************/


bool memory_c::store_load_forwarding(Op *mem_op)
{
			if( mem_op->mem_type == MEM_LD ) {
				printf( "will piggyback\n" );
				m_mshr_entry_s * entry = search_matching_mshr( mem_op->ld_vaddr );
				if( entry != NULL ) {
  					list<Op *>::const_iterator cii;
	  				for (cii= entry->req_ops.begin() ; cii != entry->req_ops.end(); cii++) {
						Op *tempOp = (*cii);
						printf("tempOp->inst_id: %d\n", tempOp->inst_id);
						if( tempOp->mem_type == MEM_ST && 
							 mem_op->ld_vaddr >= tempOp->st_vaddr && 
							 tempOp->mem_write_size +tempOp->st_vaddr >= mem_op->mem_read_size + mem_op->ld_vaddr ) {
							printf("store load forward successful\n");
							return true;
						}
					}
				}
			}
	return false;
  /* For Lab #2, you need to fill out this function */ 

}
