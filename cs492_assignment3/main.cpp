#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <string.h>
#include <fstream>
#include <libgen.h>
#include <stack>
#include <array>
#include <vector>
#include <math.h>
#include <cassert>

using namespace std;

// Node for Ldisk linked-list
struct D_node {
	int start; 
	int end;
	bool used;
	D_node* next;
	D_node* prev;
};

// Node for Lfile linked-list
struct F_node {
	int start_addr = -1;
	int bytes_used = 0;
	F_node* next = NULL;
	F_node* prev = NULL;
};

// Node for Hierarchical Directory Tree G
struct G_node {
	char* name;
	bool dir = 1;
	vector<G_node*> contents;
	char* abspath;
	G_node* parent;

	int size = -1;
	time_t timestamp;
	F_node* lfile = NULL;
};

G_node* groot;
D_node* Ldisk;
int fragmentation;
int block_size;

// mode: 0 means print the dir (print directory tree), 1 means prfiles, 2 means prdisk
void printG(int mode) {
	if(mode != 0 && mode != 1 && mode != 2) {
		cout << "Error: printG: not a valid mode" << endl;
		return;
	}
	if(mode == 2) {
		cout << "~~~PRDISK~~~" << endl;
		D_node* cur = Ldisk;
		while(cur != NULL) {
			if(cur->used) {
				if(cur->start == cur->end) {
					cout << "In use: " << cur->start << endl;
				}
				else {
					cout << "In use: " << cur->start << "-" << cur->end << endl;
				}
			}
			else {
				if(cur->start == cur->end) {
					cout << "Free: " << cur->start << endl;
				}
				else {
					cout << "Free: " << cur->start << "-" << cur->end << endl;
				}
			}
			cur = cur->next;
		}
		cout << "fragmentation: " << fragmentation << " bytes\n" << endl;
	}
	if(mode == 0) {
		cout << "\n~~~G Tree~~~\n\n[DIR] root" << endl;
	}
	if(mode == 1) {
		cout << "~~~PRFILES~~~" << endl;
	}
	stack<G_node*> left_to_print;
	G_node* cur = groot;
	vector<G_node*> contents = cur->contents;
	for(int i = 0; i < contents.size(); i++) {
		G_node* item = contents.at(i);
		if(mode == 0) {
			cout << item->name << endl;
		}
		else if(mode == 1) {
			if(item->dir == 0) {
				cout << "\n" << item->abspath << ":" << endl;
				cout << item->name << " size = " << item->size << endl;
				cout << item->name << " timestamp = " << item->timestamp << endl;
				if(item->lfile->start_addr == -1) {
					cout << "Currently no blocks allocated to " << item->name << endl;
				}
				else {
					cout << "Block addresses allocated to " << item->name << ":" << endl;
					vector<int> block_addrs;
					F_node* cur = item->lfile;
					while(cur != NULL) {
						int block_id = floor(cur->start_addr / block_size);
						block_addrs.push_back(block_id);
						cur = cur->next;
					}
					for(int i = 0; i < block_addrs.size()-1; i++) {
						cout << block_addrs.at(i) << ", ";
					}
					cout << block_addrs.at(block_addrs.size()-1) << endl;
				}
			}
		}
		if(item->dir) {
			left_to_print.push(contents.at(i));
		}
	}
	if(mode == 0) {
		cout << contents.size() << " items" << endl;
	}
	while(!left_to_print.empty()) {
		cur = left_to_print.top();
		if(mode == 0) {
			cout << "\n[DIR] " << cur->name << endl;
		}
		left_to_print.pop();
		contents = cur->contents;
		for(int i = 0; i < contents.size(); i++) {
			G_node* item = contents.at(i);
			if(mode == 0) {
				cout << item->name << endl;
			}
			else if(mode == 1) {
				if(item->dir == 0) {
					cout << "\n" << item->abspath << ":" << endl;
					cout << item->name << " size = " << item->size << endl;
					cout << item->name << " timestamp = " << item->timestamp << endl;
					if(item->lfile->start_addr == -1) {
						cout << "Currently no blocks allocated to " << item->name << endl;
					}
					else {
						cout << "Block addresses allocated to " << item->name << ":" << endl;
						vector<int> block_addrs;
						F_node* cur = item->lfile;
						while(cur != NULL) {
							int block_id = floor(cur->start_addr / block_size);
							block_addrs.push_back(block_id);
							cur = cur->next;
						}
						for(int i = 0; i < block_addrs.size()-1; i++) {
							cout << block_addrs.at(i) << ", ";
						}
						cout << block_addrs.at(block_addrs.size()-1) << endl;
					}
				}
			}
			if(item->dir) {
				left_to_print.push(contents.at(i));
			}
		}
		if(mode == 0) {
			cout << contents.size() << " items" << endl;
		}
	}
}

// G_node* findGnode(char* nodepath) {
// 	// Build stack of directory names
// 	char* path = strdup(nodepath);
// 	stack<char*> trail;
// 	while(strcmp(path, ".") != 0) {
// 		trail.push(strdup(basename(path)));
// 		char *newpath = strdup(dirname(path));
// 		free(path);
// 		path = newpath;
// 	}

// 	// Traverse stack to bring cursor G_node to the parent directory
// 	G_node* cur = groot;
// 	bool found = 0;
// 	if(trail.empty()) {
// 		found = 1;
// 	}
// 	while(!trail.empty()) {
// 		vector<G_node*> contents = cur->contents;
// 		char* next = trail.top();
// 		trail.pop();
// 		for(int i = 0; i < contents.size(); i++) {
// 			if(strcmp(contents.at(i)->name, next) == 0) {
// 				cur = contents.at(i);
// 				found = 1;
// 				break;
// 			}
// 		}
// 	}
// 	if(found) {
// 		return cur;
// 	}
// 	else {
// 		cout << "Path: " << nodepath << " is not a valid path to a G_node." << endl;
// 		return NULL;
// 	}
// }

G_node* findGnode(char* nodepath) {
	// Build stack of directory names
	char* item = strdup(basename(nodepath));
	char* path = strdup(dirname(nodepath));
	stack<char*> trail;
	while(strcmp(path, ".") != 0) {
		trail.push(strdup(basename(path)));
		char *newpath = strdup(dirname(path));
		free(path);
		path = newpath;
	}

	// Traverse stack to bring cursor G_node to the parent directory
	G_node* cur = groot;
	bool found = 0;
	// if(trail.empty()) {
	// 	found = 1;
	// }
	while(!trail.empty()) {
		vector<G_node*> contents = cur->contents;
		char* next = trail.top();
		trail.pop();
		for(int i = 0; i < contents.size(); i++) {
			if(strcmp(contents.at(i)->name, next) == 0) {
				cur = contents.at(i);
				break;
			}
		}
	}
	vector<G_node*> contents = cur->contents;
	for(int i = 0; i < contents.size(); i++) {
		if(strcmp(contents.at(i)->name, item) == 0) {
			cur = contents.at(i);
			return cur;
		}
	}
	// cout << "Path: " << nodepath << " is not a valid path to a G_node." << endl;
	return NULL;
}


int allocateBlock() {
	int blockid = -1;
	D_node* cur = Ldisk;
	while(cur->used) {
		cur = cur->next;
		if(cur == NULL) {
			cout << "Out of space" << endl;
			return blockid;
		}
	}
	blockid = cur->start;
	cur->start += 1;
	if(cur->prev != NULL) {
		cur->prev->end += 1;
	}
	else {
		D_node* new_block = new D_node;
		new_block->start = 0;
		new_block->end = 0;
		new_block->used = 1;
		new_block->prev = NULL;
		new_block->next = cur;
		cur->prev = new_block;
		Ldisk = new_block;
	}
	fragmentation += block_size;
	return blockid;
}

void allocateBytes(F_node* f, int num_bytes, G_node* gnode) {
	while(f->next != NULL) {
		f = f->next;
	}
	while(num_bytes > 0) {
		int free_block_space = block_size - f->bytes_used;
		if(free_block_space >= num_bytes) {
			f->bytes_used += num_bytes;
			gnode->size += num_bytes;
			fragmentation -= num_bytes;
			num_bytes -= num_bytes;
		}
		else {
			fragmentation -= free_block_space;
			num_bytes -= free_block_space;
			gnode->size += free_block_space;
			f->bytes_used = block_size;
			int newblockid = allocateBlock();
			if(newblockid == -1) {
				cout << "Out of space" << endl;
				return;
			}
			F_node* fnode = new F_node;
			fnode->start_addr = newblockid * block_size;
			f->next = fnode;
			fnode->prev = f;
			f = f->next;
		}
	}
}

G_node* findItemInDir(G_node* cwd, char* item_name, bool dir) {
	vector<G_node*> contents = cwd->contents;
	for(int i = 0; i < contents.size(); i++) {
		if(strcmp(contents.at(i)->name, item_name) == 0 && contents.at(i)->dir == dir) {
			return contents.at(i);
		}
	}
	return NULL;
}

int deallocateBlock(int blockid) {
	if(blockid < 0) {
		cout << "Please provide a valid blockid to deallocate." << endl;
		return -1;
	}
	D_node* cur = Ldisk;
	while(!(blockid >= cur->start && blockid <= cur->end)) {
		cur = cur->next;
		if(cur == NULL) {
			cout << "Please provide a valid blockid to deallocate." << endl;
			return -1;
		}
	}
	if(cur->used == 0) {
		cout << "Block " << blockid << " is already free" << endl;
		return -1;
	}
	if(blockid == cur->start && blockid == cur->end) {
		cur->used = 0;
		if(cur->prev != NULL && cur->next != NULL && cur->next->used == 0 && cur->prev->used == 0) {
			cur->prev->end = cur->next->end;
			cur->prev->next = cur->next->next;
			if(cur->next->next != NULL) {
				cur->next->next->prev = cur->prev;
			}
			delete(cur->next);
			delete(cur);
		}
		else if(cur->prev != NULL && cur->prev->used == 0) {
			cur->prev->next = cur->next;
			cur->prev->end += 1;
			if(cur->next != NULL) {
				cur->next->prev = cur->prev;
			}
			delete(cur);
		}
		else if(cur->next != NULL && cur->next->used == 0) {
			cur->next->prev = cur->prev;
			cur->next->start -= 1;
			if(cur->prev != NULL) {
				cur->prev->next = cur->next;
			}
			else {
				Ldisk = cur->next;
			}
			delete(cur);
		}
	}
	else if(blockid == cur->start) {
		if(cur->prev == NULL) {
			D_node* new_disk_node = new D_node;
			new_disk_node->start = 0;
			new_disk_node->end = 0;
			new_disk_node->used = 0;
			new_disk_node->prev = NULL;
			new_disk_node->next = cur;
			cur->start += 1;
			cur->prev= new_disk_node;
		}
		else {
			assert(cur->prev->used == 0);
			cur->prev->end += 1;
			cur->start += 1;
		}
	}
	else if(blockid == cur->end) {
		if(cur->next == NULL) {
			D_node* new_disk_node = new D_node;
			new_disk_node->start = cur->end;
			new_disk_node->end = cur->end;
			new_disk_node->used = 0;
			new_disk_node->prev = cur;
			new_disk_node->next = NULL;
			cur->next = new_disk_node;
			cur->end -= 1;
		}
		else {
			assert(cur->next->used == 0);
			cur->end -= 1;
			cur->next->start -= 1;
		}
	}
	else {
		D_node* freed_block = new D_node;
		D_node* new_block = new D_node;
		freed_block->start = blockid;
		freed_block->end = blockid;
		freed_block->used = 0;
		freed_block->prev = cur;
		freed_block->next = new_block;
		new_block->start = blockid + 1;
		new_block->end = cur->end;
		new_block->used = 1;
		new_block->prev = freed_block;
		new_block->next = cur->next;
		cur->end = blockid - 1;
		cur->next = freed_block;
	}
	return 1;
}

void removeBytes(char* fullpath, int num_bytes) {
	if(num_bytes == 0) {
		cout << "No point removing 0 bytes" << endl;
		return;
	}
	G_node* f = findGnode(fullpath);
	if(f == NULL) {
		cout << fullpath << " is not a valid file." << endl;
		return;
	}
	F_node* cur = f->lfile;
	if(cur == NULL) {
		cout << "ERROR: removeBytes: " << fullpath << " is not a valid file" << endl;
		return;
	}
	if(cur->start_addr == -1) {
		cout << "File has no bytes to remove" << endl;
		return;
	}
	f->timestamp = time(NULL);
	while(cur->next != NULL) {
		cur = cur->next;
	}
	while(num_bytes > 0) {
		if(cur->bytes_used >= num_bytes) {
			cur->bytes_used -= num_bytes;
			fragmentation += num_bytes;
			f->size -= num_bytes;
			num_bytes -= num_bytes;
			if(cur->bytes_used == 0) {
				if(cur->prev == NULL) {
					fragmentation -= block_size;
					int success = deallocateBlock(floor(cur->start_addr / block_size));
					if(success == -1) {
						cout << "Was not able to deallocate block " << floor(cur->start_addr / block_size) << endl;
						return;
					}
					cur->start_addr = -1;
				}
				else {
					fragmentation -= block_size;
					int success = deallocateBlock(floor(cur->start_addr/ block_size));
					if(success == -1) {
						cout << "Was not able to deallocate block " << floor(cur->start_addr / block_size) << endl;
						return;
					}
					cur = cur->prev;
					delete(cur->next);
					cur->next = NULL;
				}
			}
		}
		else {
			fragmentation -= (block_size - cur->bytes_used);
			f->size -= cur->bytes_used;
			num_bytes -= cur->bytes_used;
			cur->bytes_used = 0;
			if(cur->prev == NULL) {
				int success = deallocateBlock(floor(cur->start_addr / block_size));
				if(success == -1) {
					cout << "Was not able to deallocate block " << floor(cur->start_addr / block_size) << endl;
					return;
				}
				cur->start_addr = -1;
				cout << "File ran out of bytes to remove" << endl;
				return;
			}
			else {
				int success = deallocateBlock(floor(cur->start_addr / block_size));
				if(success == -1) {
					cout << "Was not able to deallocate block " << floor(cur->start_addr / block_size) << endl;
					return;
				}
				cur = cur->prev;
				delete(cur->next);
				cur->next = NULL;
			}
		}
	}
}

void deleteNode(char* fullpath) {
	G_node* node = findGnode(fullpath);
	if(node == NULL) {
		cout << "No such file or directory" << endl;
		return;
	}
	G_node* parent = node->parent;
	// Case if node is a directory
	if(node->dir) {
		if(!node->contents.empty()) {
			cout << "Directory not empty" << endl;
			return;
		}
		G_node* parent = node->parent;
		for(auto it = parent->contents.begin(); it != parent->contents.end(); it++) {
			if(*it == node) {
				parent->contents.erase(it);
			}
		}
		delete(node);
	}
	else {
		F_node* cur = node->lfile;
		while(cur->next != NULL) {
			cur = cur->next;
		}
		while(cur->prev != NULL) {
			fragmentation -= block_size - cur->bytes_used;
			deallocateBlock(floor(cur->start_addr / block_size));
			cur = cur->prev;
			delete(cur->next);
		}
		fragmentation -= block_size - cur->bytes_used;
		deallocateBlock(floor(cur->start_addr / block_size));
		for(auto it = parent->contents.begin(); it != parent->contents.end(); ) {
			if(*it == node) {
				parent->contents.erase(it);
				break;
			}
			++it;
		}
		parent->timestamp = time(NULL);
		delete(cur);
	}
}

void appendBytes(char* fullpath, int num_bytes) {
	if(num_bytes == 0) {
		cout << "No point appending 0 bytes" << endl;
		return;
	}
	G_node* f = findGnode(fullpath);
	if(f == NULL) {
		cout << fullpath << " is not a valid file." << endl;
		return;
	}
	F_node* cur = f->lfile;
	if(cur->start_addr == -1) {
		int newblockid = allocateBlock();
		if(newblockid == -1) {
			return;
		}
		cur->start_addr = newblockid * block_size;
		allocateBytes(cur, num_bytes, f);
	}
	else {
		allocateBytes(cur, num_bytes, f);
	}
	f->timestamp = time(NULL);
}

void addGnode(char* fullpath, bool dir, int size) {
	G_node* gnode = new G_node;
	gnode->abspath = strdup(fullpath);
	char* newname = strdup(basename(fullpath));
	gnode->name = newname;
	if(!dir) {
		gnode->size = 0;
		gnode->timestamp = time(NULL);
		gnode->dir = 0;
		F_node* fnode = new F_node;
		gnode->lfile = fnode;
	}

	// Build stack of directory names
	char* path = strdup(dirname(fullpath));
	stack<char*> trail;
	while(strcmp(path, ".") != 0) {
		trail.push(strdup(basename(path)));
		char *newpath = strdup(dirname(path));
		free(path);
		path = newpath;
	}

	// Traverse stack to bring cursor G_node to the parent directory
	G_node* cur = groot;
	bool found = 0;
	if(trail.empty()) {
		found = 1;
	}
	while(!trail.empty()) {
		vector<G_node*> contents = cur->contents;
		char* next = trail.top();
		trail.pop();
		for(int i = 0; i < contents.size(); i++) {
			if(strcmp(contents.at(i)->name, next) == 0) {
				cur = contents.at(i);
				found = 1;
				break;
			}
		}
	}
	if(found) {
		// Add the new directory G_node to its parent
		gnode->parent = cur;
		cur->contents.push_back(gnode);
	}
	else {
		cout << "Path: " << fullpath << " is not a valid path." << endl;
	}
}

void printContents(G_node* dir) {
	vector<G_node*> contents = dir->contents;
	for(int i = 0; i < contents.size(); i++) {
		cout << contents.at(i)->name << endl;
	}
}

void deallocateEverything() {
	stack<G_node*> gtree;
	stack<G_node*> helper;
	gtree.push(groot);
	for(int i = 0; i < groot->contents.size(); i++) {
		gtree.push(groot->contents.at(i));
		helper.push(groot->contents.at(i));
	}
	while(!helper.empty()) {
		G_node* cur = helper.top();
		vector<G_node*> cc = cur->contents;
		helper.pop();
		while(!cur->dir) {
			cur = helper.top();
			cc = cur->contents;
			helper.pop();
		}
		for(int i = 0; i < cc.size(); i++) {
			gtree.push(cc.at(i));
			helper.push(cc.at(i));
		}
	}
	while(!gtree.empty()) {
		G_node* cur = gtree.top();
		gtree.pop();
		delete(cur);
	}
}


int main(int argc, char *argv[]) {
	groot = new G_node;
	groot->name = (char*)"groot";
	groot->abspath = (char*)"./";
	groot->parent = NULL;
	cout << "GROOT" << &groot << endl;

	fragmentation = 0;

	cout << "USAGE: Write the parameter directly after the flag! (e.g. ./main -f file_list.txt -d dir_list.txt -s 512 -b 16)" << endl;

	// Parse command line arguments
	char* file_list;
	char* dir_list;
	int disk_size;
	for(int i = 0; i < argc; i++) {
		if(strcmp(argv[i], "-f") == 0) {
			file_list = argv[i+1];
		}
		else if(strcmp(argv[i], "-d") == 0) {
			dir_list = argv[i+1];
		}
		else if(strcmp(argv[i], "-s") == 0) {
			disk_size = atoi(argv[i+1]);
		}
		else if(strcmp(argv[i], "-b") == 0) {
			block_size = atoi(argv[i+1]);
		}
	}

	int num_blocks = floor(disk_size/block_size);

	Ldisk = new D_node;
	int block_ids[num_blocks];
	Ldisk->start = 0;
	Ldisk->end = num_blocks-1;
	Ldisk->used = 0;
	Ldisk->next = NULL;
	Ldisk->prev = NULL;

	cout << "file_list == " << file_list << endl;
	cout << "dir_list == " << dir_list << endl;
	cout << "disk_size == " << disk_size << endl;
	cout << "block_size == " << block_size << endl;

	// Parse directory list
	cout << "\nLoading directory structure:" << endl;
	ifstream dir_stream(dir_list);
	char dir_path_name[1024];

	if(!dir_stream) {
		cout << "Error opening the directory list, are you sure you input the filename correctly? Make sure to include the .txt extension." << endl;
		exit(1);
	}
	while(dir_stream >> dir_path_name) {

		if(strcmp(dir_path_name, "./") == 0) {
			continue;
		}
		cout << "Adding directory " << dir_path_name << " to G tree" << endl;
		addGnode(dir_path_name, 1, 0);
	}

	// Parse file list
	cout << "\nLoading files:" << endl;
	ifstream file_stream(file_list);
	char junk[1024];
	int file_size;
	char file_path_name[1024];
	if(!file_stream) {
		cout << "Error opening the file list, are you sure you input the filename correctly? Make sure to include the .txt extension." << endl;
		exit(1);
	}
	while(file_stream >> junk >> junk >> junk >> junk >> junk >> junk >> file_size >> junk >> junk >> junk >> file_path_name) {
		cout << file_path_name << " is " << file_size << " bytes large!" << endl;
		addGnode(file_path_name, 0, file_size);
		appendBytes(file_path_name, file_size);
	}
	cout << "\n" << endl;

	printG(0);
	cout << "\n" << endl;
	printG(1);
	cout << "\n" << endl;
	printG(2);

	cout << "Starting the REPL, write some commands!" << endl;

	string line;
	G_node* cwd = groot;
	while(true) {
		cout << "[CWD] " << cwd->abspath << endl;
		getline(cin, line);
		int space_ind = (int)line.find_first_of(32,0); // 32 == space
		string command;
		if(space_ind == string::npos) {
			command = line;
		}
		else {
			command = line.substr(0,space_ind);
		}
		if(command == "exit") {
			break;
		}
		else if(command == "cd") {
			string dir = line.substr(space_ind+1, string::npos);

			if(dir == "..") {
				if(strcmp(cwd->name, "groot") != 0) {
					cwd = cwd->parent;
				}
				continue;
			}

			string cwdpath(cwd->abspath);
			if(cwdpath.at(cwdpath.length()-1) != '/') {
				cwdpath.append("/");
			}
			string fullpath = cwdpath + dir;

			G_node* new_dir = findGnode((char*)fullpath.c_str());
			if(new_dir != NULL && new_dir->dir) {
				cwd = new_dir;
			}
			else {
				cout << "cd: " << dir << ": No such directory" << endl;
			}
		}
		else if(command == "cd..") {
			if(strcmp(cwd->name, "groot") != 0) {
				cwd = cwd->parent;
			}
		}
		else if(command == "ls") {
			cout << "\n";
			printContents(cwd);
			cout << "\n";
		}
		else if(command == "mkdir") {
			string dir_name = line.substr(space_ind+1, string::npos);
			string path(cwd->abspath);
			string new_dir = path + dir_name;
			addGnode((char*)new_dir.c_str(), 1, 0);
		}
		else if(command == "create") {
			string file_name = line.substr(space_ind+1, string::npos);
			string path(cwd->abspath);
			string new_file = path + file_name;
			addGnode((char*)new_file.c_str(), 0, 0);
		}
		else if(command == "dir") {
			printG(0);
		}
		else if(command == "prfiles") {
			printG(1);
		}
		else if(command == "prdisk") {
			printG(2);
		}
		else if(command == "append") {
			int space_2 = line.find_first_of(32 ,space_ind+1);
			if(space_2 == string::npos) {
				cout << "Usage: remove [name] [bytes]" << endl;
				continue;
			}
			string file_name = line.substr(space_ind+1, space_2-space_ind-1);
			int num_bytes = stoi(line.substr(space_2, string::npos));
			string cwdpath(cwd->abspath);
			if(cwdpath.at(cwdpath.length()-1) != '/') {
				cwdpath.append("/");
			}
			string fullpath = cwdpath + file_name;
			appendBytes((char*)fullpath.c_str(), num_bytes);
		}
		else if(command == "remove") {
			int space_2 = line.find_first_of(32 ,space_ind+1);
			if(space_2 == string::npos) {
				cout << "Usage: remove [name] [bytes]" << endl;
				continue;
			}
			string file_name = line.substr(space_ind+1, space_2-space_ind-1);
			int num_bytes = stoi(line.substr(space_2, string::npos));
			string cwdpath(cwd->abspath);
			if(cwdpath.at(cwdpath.length()-1) != '/') {
				cwdpath.append("/");
			}
			string fullpath = cwdpath + file_name;
			removeBytes((char*)fullpath.c_str(), num_bytes);
		}
		else if(command == "delete") {
			string file_name = line.substr(space_ind+1, string::npos);
			string path(cwd->abspath);
			if(path.at(path.length()-1) != '/') {
				path.append("/");
			}
			string new_file = path + file_name;
			deleteNode((char*)new_file.c_str());
		}
		else {
			cout << "Not a valid command" << endl;
		}
	}
}