// Brendan von Hofe & Eric Zhen
// We pledge our honor that we have abided by the Stevens Honor System.

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <math.h>
#include <string.h>

using namespace std;

int page_swaps;
int page_faults;
unsigned long int mem_counter;
int hand = 0;

struct Page {
	int page_num;
	bool valid;
	bool referenced;
	unsigned long int last_accessed;
	unsigned long int loaded_at;
};

Page* getPage(int abs_page, int *chkpts, vector< vector<Page> > &page_tables, int num_processes) {
	if(abs_page >= chkpts[num_processes-1]) {
		int process_table = num_processes - 1;
		int rel_page = abs_page - chkpts[process_table];
		return &page_tables.at(process_table).at(rel_page);
	}
	for(int i = 1; i < num_processes; i++) {
		if(chkpts[i] > abs_page) {
			int process_table = i-1;
			int rel_page = abs_page - chkpts[process_table];
			return &page_tables.at(process_table).at(rel_page);
		}
	}
	return NULL;
}

void swapPageFIFO(int *memory, vector< vector<Page> > &page_tables, int* page_table_chkpts, int num_processes, int num_pages, Page *page_needed, bool pre) {
	Page* oldest_page = getPage(memory[0], page_table_chkpts, page_tables, num_processes);
	unsigned long int oldest_page_time = oldest_page->loaded_at;
	int oldest_page_loc = 0;
	for(int i = 1; i < num_pages; i++) {
		Page* temp = getPage(memory[i], page_table_chkpts, page_tables, num_processes);
		if(temp->loaded_at < oldest_page_time) {
			oldest_page = temp;
			oldest_page_time = oldest_page->loaded_at;
			oldest_page_loc = i;
		}
	}
		// cout << "SWAPPING PAGE: " << oldest_page->page_num << " WITH: " << page_needed->page_num << endl;
		// cout << "The old page was loaded at: " << oldest_page->loaded_at << "\n" << endl;
	memory[oldest_page_loc] = page_needed->page_num;
	page_needed->loaded_at = mem_counter;
	if(!pre) {
		page_needed->referenced = 1;
		page_needed->last_accessed = mem_counter;
		page_faults += 1;
	}
	page_needed->valid = 1;
	oldest_page->valid = 0;
	mem_counter += 1;
	page_swaps += 1;
}

void swapPageLRU(int *memory, vector< vector<Page> > &page_tables, int* page_table_chkpts, int num_processes, int num_pages, Page *page_needed, bool pre) {
	Page* oldest_page = getPage(memory[0], page_table_chkpts, page_tables, num_processes);
	unsigned long int oldest_page_time = oldest_page->last_accessed;
	int oldest_page_loc = 0;
	for(int i = 1; i < num_pages; i++) {
		Page* temp = getPage(memory[i], page_table_chkpts, page_tables, num_processes);
		if(temp->last_accessed < oldest_page_time) {
			oldest_page = temp;
			oldest_page_time = oldest_page->last_accessed;
			oldest_page_loc = i;
		}
	}
		// cout << "SWAPPING PAGE: " << oldest_page->page_num << " WITH: " << page_needed->page_num << endl;
		// cout << "The old page was loaded at: " << oldest_page->loaded_at << "\n" << endl;
	memory[oldest_page_loc] = page_needed->page_num;
	page_needed->loaded_at = mem_counter;
	if(!pre) {
		page_needed->referenced = 1;
		page_needed->last_accessed = mem_counter;
		page_faults += 1;
	}
	page_needed->valid = 1;
	oldest_page->valid = 0;
	mem_counter += 1;
	page_swaps += 1;
}

void swapPageClock(int *memory, vector< vector<Page> > &page_tables, int* page_table_chkpts, int num_processes, int num_pages, Page *page_needed, bool pre) {
	while(true) {
		Page* page_hand = getPage(memory[hand], page_table_chkpts, page_tables, num_processes);
		if(page_hand->referenced == 0) {
			memory[hand] = page_needed->page_num;
			page_needed->loaded_at = mem_counter;
			if(!pre) {
				page_needed->referenced = 1;
				page_needed->last_accessed = mem_counter;
				page_faults += 1;
			}
			page_needed->valid = 1;
			page_hand->valid = 0;
			mem_counter += 1;
			page_swaps += 1;
			hand += 1;
			if(hand == num_pages) {
				hand -= num_pages;
			}
			break;
		}
		else {
			page_hand->referenced = 0;
			hand += 1;
			if(hand == num_pages) {
				hand -= num_pages;
			}
		}
	}
}

int lookForEmptySpace(int *memory, int num_pages) {
	for(int i = 0; i < num_pages; i++) {
		if(memory[i] == -1) {
			return i;
		}
	}
	return -1;
}

int main(int argc, char *argv[]) {
	mem_counter = 0;
	int num_processes = 0;

	// Read in arguments
	char* plist_path = argv[1];
	cout << "plist_path: " << plist_path << endl;
	char* ptrace_path = argv[2];
	cout << "ptrace_path: " << ptrace_path << endl;
	float page_size = atoi(argv[3]);
	cout << "Page size: " << page_size << endl;
	char* replacement_alg = argv[4];
	// Check for good argument values with replacement algorithm and prepaging
	if(strcmp(replacement_alg, "FIFO") != 0 && 
		strcmp(replacement_alg, "LRU") != 0 &&
		strcmp(replacement_alg, "Clock") != 0) {
		cout << "Please input either FIFO, LRU, or Clock as the replacement algorithm. (case-sensitive)" << endl;
		exit(1);
	}
	cout << "Replacement algorithm: " << replacement_alg << endl;
	bool prepaging;
	if(*argv[5] == '+') {
		prepaging = true;
	}
	else if(*argv[5] == '-') {
		prepaging = false;
	}
	else {
		cout << "Please give a valid sign for prepaging, either + or -" << endl;
		exit(1);
	}
	cout << "Prepaging: " << prepaging << endl;

	// Calculate how many number of pages will fit into memory and initialize memory values to -1
	int num_pages = 512 / page_size;
	int memory[num_pages];
	for(int i = 0; i < num_pages; i++) {
		memory[i] = -1;
	}

	// Count number of processes
	ifstream count_procs(plist_path);
	int a, b;
	while(count_procs >> a >> b) {
		num_processes += 1;
	}
	cout << "Number of processes: " << num_processes << endl;

	// Pages are struct, page tables are an array of pages, and a vector contains all the page tables
	vector < vector<Page> > page_tables (num_processes);

	// Load the page tables from plist
	ifstream plist(plist_path);
	int proc, mem_needed; // Buffer variables
	int page_number = 0; // Counter to maintain unique page numbers
	int page_table_sizes[num_processes];
	int page_table_chkpts[num_processes]; // For finding pages from absolute page numbers efficiently. Stores absolute page number of first page in each page table.
	page_table_chkpts[0] = 0;
	if(!plist) {
		cout << "Error opening plist, are you sure you input the filename correctly? Make sure to include the .txt extension." << endl;
		exit(1);
	}
	// Read in processes and associated memory needs
	while(plist >> proc >> mem_needed) {
		int page_table_size = ceil(mem_needed / page_size) + 1;
		page_table_sizes[proc] = page_table_size;

		// Populate checkpoints array
		if(proc != 9) {
			page_table_chkpts[proc+1] = page_table_chkpts[proc] + page_table_size;
		}

		// Create page table and populate with pages
		vector <Page> page_table (page_table_size);
		for(int i = 0; i < page_table_size; i++) {
			Page page = *new Page;
			page.page_num = page_number;
			page.valid = 0;
			page.last_accessed = -1;
			page.referenced = -1;
			page_number += 1;
			page_table.at(i) = page;
		}
		// Add page table to vector
		page_tables.at(proc) = page_table;
		// cout << "Page num of 0th page in process " << proc << ": " << page_table[0].page_num << endl;
	}

	int ppp = num_pages / num_processes; // pages per process for initial load
	// Initial load of process pages into memory
	for(int p = 0; p < num_processes; p++) {
		for(int i = 0; i < ppp; i++) {
			if(i > page_table_sizes[p]) {
				// Leave memory locations blank if process is smaller than default load size
				break;
			}
			memory[ppp*p + i] = page_tables.at(p).at(i).page_num;
			page_tables.at(p).at(i).valid = 1;
			page_tables.at(p).at(i).loaded_at = mem_counter;
			page_tables.at(p).at(i).last_accessed = mem_counter;
			page_tables.at(p).at(i).referenced = 0;
			mem_counter += 1;
		}
	}


	page_swaps = 0; // Differentiating between page swaps and page faults because of pre-paging
	page_faults = 0;

	ifstream ptrace(ptrace_path);
	if(!ptrace) {
		cout << "Error opening ptrace, are you sure you input the filename correctly? Make sure to include the .txt extension." << endl;
		exit(1);
	}
	// Run programs in ptrace
	while(ptrace >> proc >> mem_needed) {
		int rel_page = ceil(mem_needed / page_size);
		Page* page_needed = &page_tables.at(proc).at(rel_page);

		// cout << "Process " << proc << " is attempting to access memory location: " << mem_needed << " in page: " << rel_page << endl;
		// cout << "The mem location should be in page: " << page_needed->page_num << endl;

		// Check if page is already in memory
		if(page_needed->valid == 1) {
			// cout << "Page was already loaded!\n" << endl;
			page_needed->last_accessed = mem_counter;
			page_needed->referenced = 1;
			mem_counter += 1;
			continue;
		}

		// Check memory for free space
		int space = lookForEmptySpace(memory, num_pages);
		if(space != -1) {
			// cout << "Found an empty space in memory! How luck!\n" << endl;
			memory[space] = page_needed->page_num;
			page_needed->last_accessed = mem_counter;
			page_needed->referenced = 1;
			page_needed->valid = 1;
			mem_counter += 1;
			continue;
		}

		// Swap pages out
		if(strcmp(replacement_alg, "FIFO") == 0) {
			swapPageFIFO(memory, page_tables, page_table_chkpts, num_processes, num_pages, page_needed, 0);
		}
		else if(strcmp(replacement_alg, "LRU") == 0) {
			swapPageLRU(memory, page_tables, page_table_chkpts, num_processes, num_pages, page_needed, 0);
		}
		else if(strcmp(replacement_alg, "Clock") == 0) {
			swapPageClock(memory, page_tables, page_table_chkpts, num_processes, num_pages, page_needed, 0);
		}
		// Load another page if prepaging is enabled
		if(prepaging) {
			bool no_pages_to_load = 0; // Flag in case there are no more contiguous pages to load from this process
			int next_page_num = rel_page + 1;
			Page *next_page = NULL;
			if(next_page_num >= page_table_sizes[proc]) { // No more pages to load for this process in a contiguous segment
				no_pages_to_load = 1;
			}
			else {
				next_page = &page_tables.at(proc).at(next_page_num);
				while(next_page->valid == 1) {
					next_page_num += 1;
					if(next_page_num >= page_table_sizes[proc]) {
						no_pages_to_load = 1;
						break;
					}
					next_page = &page_tables.at(proc).at(next_page_num);
				}
			}
			if(!no_pages_to_load && next_page_num < page_table_sizes[proc]) {
				int space = lookForEmptySpace(memory, num_pages);
				if(space != -1) {
					memory[space] = next_page->page_num;
					page_needed->valid = 1;
					mem_counter += 1;
					continue;
				}
				if(strcmp(replacement_alg, "FIFO") == 0) {
					swapPageFIFO(memory, page_tables, page_table_chkpts, num_processes, num_pages, next_page, 1);
				}
				else if(strcmp(replacement_alg, "LRU") == 0) {
					swapPageLRU(memory, page_tables, page_table_chkpts, num_processes, num_pages, next_page, 1);
				}
				else if(strcmp(replacement_alg, "Clock") == 0) {
					swapPageClock(memory, page_tables, page_table_chkpts, num_processes, num_pages, next_page, 1);
				}
			}
		}

	}
	ofstream stats;
	stats.open("stats.txt", ofstream::app);
	stats << replacement_alg << "," << (int)page_size << "," << prepaging << "," << page_swaps << "," << page_faults << "\n";
	stats.close();
	cout << "Finished operating this system with only " << page_swaps << " PAGE SWAPS!" << endl;
	cout << "Finished operating this system with only " << page_faults << " PAGE FAULTS!" << endl;


	return(0);
}