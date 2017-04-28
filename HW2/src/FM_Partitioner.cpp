#include <iostream>
#include <iomanip>
#include <fstream>
#include <unistd.h> // for getopt()
#include <cstdlib>  // for exit()
#include <string>
#include <vector>
#include <algorithm>
#include <utility>
#include <list>
#include <map>
using namespace std;

int legal_size_range;
int cut_size;

char *infilename_NET;
char *infilename_CELL;
char *output_file;
string checkOutputName;

void computing_cell_gain();
void write_solution_file();

struct CELL{
	int cell_name;
	int cell_size;
	vector<int> net_name;
	int set;//0 for no, 1 for A, 2 for B	//會改變
	bool lock;						//會改變
	int gain;						//會改變
	list<int>::iterator ptr;		//會改變
};

struct NET{
	int net_name;
	vector<int> cell_name;
	int one_net_size;
	pair<int, int> distribution; //會改變
};

struct PARTITION{
	list<int> cell_name;	//會改變
	int set_size;	//會改變
};

vector<NET> net;
vector<CELL> cell;
PARTITION set_A, set_B;	//會改變
map<int, list<int>, greater<int> > bucket_list_A, bucket_list_B;	//會改變
vector<int> move_cell_name;
int max_partial_sum_cell_name;//是真的cell_name
int max_partial_sum;

//map<int, string> check;

int choose_a_base_cell(int AorB){//未動
	map<int, list<int> >::iterator iter, iter_end;
	if(AorB == 1){
		iter = bucket_list_A.begin();
		iter_end = bucket_list_A.end();
	}else if(AorB == 2){
		iter = bucket_list_B.begin();
		iter_end = bucket_list_B.end();
	}else{
		cout<<"error"<<endl;
		return -2;
	}
	list<int>::iterator base_cell;
	int find_cell;
	while(iter != iter_end){
		base_cell = (*iter).second.begin();
		while(base_cell!=(*iter).second.end()){
			find_cell = (*base_cell)-1;
			while(cell[find_cell].cell_name != (*base_cell))	find_cell--;
			if( !(cell[find_cell].lock) ){	//cell[find_cell].cell_size
				if((AorB == 1) && ((set_B.set_size + cell[find_cell].cell_size - legal_size_range) <= (set_A.set_size - cell[find_cell].cell_size)) ){//若是選A 要移到B 的檢查
					return find_cell;
				}else if((AorB == 2) && ((set_A.set_size + cell[find_cell].cell_size - legal_size_range) <= (set_B.set_size - cell[find_cell].cell_size)) ){
					return find_cell;
				}else{
					base_cell++;
				}
			}else{
				//cout<<"Something wrong!"<<endl;
				base_cell++;
			}
		}
		iter++;
	}
	return -1;
}
void Updating_cell_gain(int base_cell, int from_block){//未用
	cell[base_cell].lock = true;	//lock the base_cell
	int local_cell;
	list<int>::iterator iter_delete;
	map<int, list<int> >::iterator iter_insert, iter_erase;
	if(from_block == 1){
		for(vector<int>::iterator i=cell[base_cell].net_name.begin();i!=cell[base_cell].net_name.end();i++){//base_cell身上的net
			if(net[(*i)-1].distribution.second == 0){
				for(vector<int>::iterator j=net[(*i)-1].cell_name.begin();j!=net[(*i)-1].cell_name.end();j++){//這些net所連到的cell
					local_cell = (*j)-1;	while(cell[local_cell].cell_name != (*j))	local_cell--;
					if(!(cell[local_cell].lock) && (cell[local_cell].set == 1)){//increment gainsof all free cells on n
						iter_erase = bucket_list_A.find(cell[local_cell].gain);
						cell[local_cell].gain++;
						iter_insert = bucket_list_A.find(cell[local_cell].gain);
						iter_delete = cell[local_cell].ptr;
						if(iter_insert != bucket_list_A.end()){//插入新的cell_name進入bucket_list_A
							(*iter_insert).second.push_back(cell[local_cell].cell_name);
							cell[local_cell].ptr = --(*iter_insert).second.end();
						}else{
							pair<int, list<int> > temp_bucket_list;
							temp_bucket_list.first = cell[local_cell].gain;
							temp_bucket_list.second.push_back(cell[local_cell].cell_name);
							bucket_list_A.insert(temp_bucket_list);
							iter_insert = bucket_list_A.find(cell[local_cell].gain);
							cell[local_cell].ptr = (*iter_insert).second.begin();
						}
						(*iter_erase).second.erase(iter_delete);//刪除舊的cell_name進入bucket_list_A
					}//increment gainsof all free cells on n
				}
			}else if(net[(*i)-1].distribution.second == 1){
				for(vector<int>::iterator j=net[(*i)-1].cell_name.begin();j!=net[(*i)-1].cell_name.end();j++){//這些net所連到的cell
					local_cell = (*j)-1;	while(cell[local_cell].cell_name != (*j))	local_cell--;
					if(!(cell[local_cell].lock) && (cell[local_cell].set == 2)){//increment gainsof all free cells on n
						iter_erase = bucket_list_B.find(cell[local_cell].gain);
						cell[local_cell].gain--;
						iter_insert = bucket_list_B.find(cell[local_cell].gain);
						iter_delete = cell[local_cell].ptr;
						if(iter_insert != bucket_list_B.end()){//插入新的cell_name進入bucket_list_A
							(*iter_insert).second.push_back(cell[local_cell].cell_name);
							cell[local_cell].ptr = --(*iter_insert).second.end();
						}else{
							pair<int, list<int> > temp_bucket_list;
							temp_bucket_list.first = cell[local_cell].gain;
							temp_bucket_list.second.push_back(cell[local_cell].cell_name);
							bucket_list_B.insert(temp_bucket_list);
							iter_insert = bucket_list_B.find(cell[local_cell].gain);
							cell[local_cell].ptr = (*iter_insert).second.begin();
						}
						(*iter_erase).second.erase(iter_delete);//刪除舊的cell_name進入bucket_list_A
					}//increment gainsof all free cells on n
				}
			}
			net[(*i)-1].distribution.first -= 1;	net[(*i)-1].distribution.second += 1;
		}
		for(vector<int>::iterator i=cell[base_cell].net_name.begin();i!=cell[base_cell].net_name.end();i++){//base_cell身上的net
			if(net[(*i)-1].distribution.first == 0){
				for(vector<int>::iterator j=net[(*i)-1].cell_name.begin();j!=net[(*i)-1].cell_name.end();j++){//這些net所連到的cell
					local_cell = (*j)-1;	while(cell[local_cell].cell_name != (*j))	local_cell--;
					if(!(cell[local_cell].lock) && (cell[local_cell].set == 2)){//increment gainsof all free cells on n
						iter_erase = bucket_list_B.find(cell[local_cell].gain);
						cell[local_cell].gain--;
						iter_insert = bucket_list_B.find(cell[local_cell].gain);
						iter_delete = cell[local_cell].ptr;
						if(iter_insert != bucket_list_B.end()){//插入新的cell_name進入bucket_list_A
							(*iter_insert).second.push_back(cell[local_cell].cell_name);
							cell[local_cell].ptr = --(*iter_insert).second.end();
						}else{
							pair<int, list<int> > temp_bucket_list;
							temp_bucket_list.first = cell[local_cell].gain;
							temp_bucket_list.second.push_back(cell[local_cell].cell_name);
							bucket_list_B.insert(temp_bucket_list);
							iter_insert = bucket_list_B.find(cell[local_cell].gain);
							cell[local_cell].ptr = (*iter_insert).second.begin();
						}
						(*iter_erase).second.erase(iter_delete);//刪除舊的cell_name進入bucket_list_A
					}//increment gainsof all free cells on n
				}
			}else if(net[(*i)-1].distribution.first == 1){
				for(vector<int>::iterator j=net[(*i)-1].cell_name.begin();j!=net[(*i)-1].cell_name.end();j++){//這些net所連到的cell
					local_cell = (*j)-1;	while(cell[local_cell].cell_name != (*j))	local_cell--;
					if(!(cell[local_cell].lock) && (cell[local_cell].set == 1)){//increment gainsof all free cells on n
						iter_erase = bucket_list_A.find(cell[local_cell].gain);
						cell[local_cell].gain++;
						iter_insert = bucket_list_A.find(cell[local_cell].gain);
						iter_delete = cell[local_cell].ptr;
						if(iter_insert != bucket_list_A.end()){//插入新的cell_name進入bucket_list_A
							(*iter_insert).second.push_back(cell[local_cell].cell_name);
							cell[local_cell].ptr = --(*iter_insert).second.end();
						}else{
							pair<int, list<int> > temp_bucket_list;
							temp_bucket_list.first = cell[local_cell].gain;
							temp_bucket_list.second.push_back(cell[local_cell].cell_name);
							bucket_list_A.insert(temp_bucket_list);
							iter_insert = bucket_list_A.find(cell[local_cell].gain);
							cell[local_cell].ptr = (*iter_insert).second.begin();
						}
						(*iter_erase).second.erase(iter_delete);//刪除舊的cell_name進入bucket_list_A
					}//increment gainsof all free cells on n
				}
			}
		}
	}else{//from_block == 2
		for(vector<int>::iterator i=cell[base_cell].net_name.begin();i!=cell[base_cell].net_name.end();i++){//base_cell身上的net
			if(net[(*i)-1].distribution.first == 0){
				for(vector<int>::iterator j=net[(*i)-1].cell_name.begin();j!=net[(*i)-1].cell_name.end();j++){//這些net所連到的cell
					local_cell = (*j)-1;	while(cell[local_cell].cell_name != (*j))	local_cell--;
					if(!(cell[local_cell].lock) && (cell[local_cell].set == 2)){//increment gainsof all free cells on n
						iter_erase = bucket_list_B.find(cell[local_cell].gain);
						cell[local_cell].gain++;
						iter_insert = bucket_list_B.find(cell[local_cell].gain);
						iter_delete = cell[local_cell].ptr;
						if(iter_insert != bucket_list_B.end()){//插入新的cell_name進入bucket_list_A
							(*iter_insert).second.push_back(cell[local_cell].cell_name);
							cell[local_cell].ptr = --(*iter_insert).second.end();
						}else{
							pair<int, list<int> > temp_bucket_list;
							temp_bucket_list.first = cell[local_cell].gain;
							temp_bucket_list.second.push_back(cell[local_cell].cell_name);
							bucket_list_B.insert(temp_bucket_list);
							iter_insert = bucket_list_B.find(cell[local_cell].gain);
							cell[local_cell].ptr = (*iter_insert).second.begin();
						}
						(*iter_erase).second.erase(iter_delete);//刪除舊的cell_name進入bucket_list_A
					}//increment gainsof all free cells on n
				}
			}else if(net[(*i)-1].distribution.first == 1){
				for(vector<int>::iterator j=net[(*i)-1].cell_name.begin();j!=net[(*i)-1].cell_name.end();j++){//這些net所連到的cell
					local_cell = (*j)-1;	while(cell[local_cell].cell_name != (*j))	local_cell--;
					if(!(cell[local_cell].lock) && (cell[local_cell].set == 1)){//increment gainsof all free cells on n
						iter_erase = bucket_list_A.find(cell[local_cell].gain);
						cell[local_cell].gain--;
						iter_insert = bucket_list_A.find(cell[local_cell].gain);
						iter_delete = cell[local_cell].ptr;
						if(iter_insert != bucket_list_A.end()){//插入新的cell_name進入bucket_list_A
							(*iter_insert).second.push_back(cell[local_cell].cell_name);
							cell[local_cell].ptr = --(*iter_insert).second.end();
						}else{
							pair<int, list<int> > temp_bucket_list;
							temp_bucket_list.first = cell[local_cell].gain;
							temp_bucket_list.second.push_back(cell[local_cell].cell_name);
							bucket_list_A.insert(temp_bucket_list);
							iter_insert = bucket_list_A.find(cell[local_cell].gain);
							cell[local_cell].ptr = (*iter_insert).second.begin();
						}
						(*iter_erase).second.erase(iter_delete);//刪除舊的cell_name進入bucket_list_A
					}//increment gainsof all free cells on n
				}
			}
			net[(*i)-1].distribution.first += 1;	net[(*i)-1].distribution.second -= 1;
		}
		for(vector<int>::iterator i=cell[base_cell].net_name.begin();i!=cell[base_cell].net_name.end();i++){//base_cell身上的net
			if(net[(*i)-1].distribution.second == 0){
				for(vector<int>::iterator j=net[(*i)-1].cell_name.begin();j!=net[(*i)-1].cell_name.end();j++){//這些net所連到的cell
					local_cell = (*j)-1;	while(cell[local_cell].cell_name != (*j))	local_cell--;
					if(!(cell[local_cell].lock) && (cell[local_cell].set == 1)){//increment gainsof all free cells on n
						iter_erase = bucket_list_A.find(cell[local_cell].gain);
						cell[local_cell].gain--;
						iter_insert = bucket_list_A.find(cell[local_cell].gain);
						iter_delete = cell[local_cell].ptr;
						if(iter_insert != bucket_list_A.end()){//插入新的cell_name進入bucket_list_A
							(*iter_insert).second.push_back(cell[local_cell].cell_name);
							cell[local_cell].ptr = --(*iter_insert).second.end();
						}else{
							pair<int, list<int> > temp_bucket_list;
							temp_bucket_list.first = cell[local_cell].gain;
							temp_bucket_list.second.push_back(cell[local_cell].cell_name);
							bucket_list_A.insert(temp_bucket_list);
							iter_insert = bucket_list_A.find(cell[local_cell].gain);
							cell[local_cell].ptr = (*iter_insert).second.begin();
						}
						(*iter_erase).second.erase(iter_delete);//刪除舊的cell_name進入bucket_list_A
					}//increment gainsof all free cells on n
				}
			}else if(net[(*i)-1].distribution.second == 1){
				for(vector<int>::iterator j=net[(*i)-1].cell_name.begin();j!=net[(*i)-1].cell_name.end();j++){//這些net所連到的cell
					local_cell = (*j)-1;	while(cell[local_cell].cell_name != (*j))	local_cell--;
					if(!(cell[local_cell].lock) && (cell[local_cell].set == 2)){//increment gainsof all free cells on n
						iter_erase = bucket_list_B.find(cell[local_cell].gain);
						cell[local_cell].gain++;
						iter_insert = bucket_list_B.find(cell[local_cell].gain);
						iter_delete = cell[local_cell].ptr;
						if(iter_insert != bucket_list_B.end()){//插入新的cell_name進入bucket_list_A
							(*iter_insert).second.push_back(cell[local_cell].cell_name);
							cell[local_cell].ptr = --(*iter_insert).second.end();
						}else{
							pair<int, list<int> > temp_bucket_list;
							temp_bucket_list.first = cell[local_cell].gain;
							temp_bucket_list.second.push_back(cell[local_cell].cell_name);
							bucket_list_B.insert(temp_bucket_list);
							iter_insert = bucket_list_B.find(cell[local_cell].gain);
							cell[local_cell].ptr = (*iter_insert).second.begin();
						}
						(*iter_erase).second.erase(iter_delete);//刪除舊的cell_name進入bucket_list_A
					}//increment gainsof all free cells on n
				}
			}
		}
	}
}
void Fake_algorithm(){//動很多
	int from_block, choose_a_base_cell_from_A, choose_a_base_cell_from_B, base_cell;
	//先嘗試將A的移到B 若違反面積則 B到A
	move_cell_name.clear();
	int currect_partial_sum = 0;
	max_partial_sum = 0;
	static int count;
	while(true){
		choose_a_base_cell_from_A = choose_a_base_cell(1);
		choose_a_base_cell_from_B = choose_a_base_cell(2);//先選出一個要換過去的cell 從bucket_list_A 和 bucket_list_B中選
		
		if(choose_a_base_cell_from_A == -1){ //and calculation set_size;
			if(choose_a_base_cell_from_B == -1){
				cout<<"Round: "<<++count<<endl;
				break;//from_block = 0;	base_cell = -1;
			}else{
				from_block = 2;	base_cell = choose_a_base_cell_from_B;
				set_A.set_size += cell[base_cell].cell_size;
				set_B.set_size -= cell[base_cell].cell_size;
			}
		}else{
			if(choose_a_base_cell_from_B == -1){
				from_block = 1;	base_cell = choose_a_base_cell_from_A;
				set_A.set_size -= cell[base_cell].cell_size;
				set_B.set_size += cell[base_cell].cell_size;
			}else if((cell[choose_a_base_cell_from_A].gain >= cell[choose_a_base_cell_from_B].gain)){//這個等於會讓我先將A移動到B多些
				from_block = 1;	base_cell = choose_a_base_cell_from_A;
				set_A.set_size -= cell[base_cell].cell_size;
				set_B.set_size += cell[base_cell].cell_size;
			}else{
				from_block = 2;	base_cell = choose_a_base_cell_from_B;
				set_A.set_size += cell[base_cell].cell_size;
				set_B.set_size -= cell[base_cell].cell_size;
			}
		}
		
		Updating_cell_gain(base_cell, from_block);
		
//-------------------------------------------------------------------------------------------------------------		
		//計算partial_sum
		move_cell_name.push_back(cell[base_cell].cell_name);
		//cout<<"currect_partial_sum: "<<currect_partial_sum<<endl;
		currect_partial_sum += cell[base_cell].gain;
		//cut_size -= cell[base_cell].gain;
		if(currect_partial_sum >= max_partial_sum){
			max_partial_sum = currect_partial_sum;
			max_partial_sum_cell_name = cell[base_cell].cell_name;
		}
	}
}
void FM_algorithm(){//未用
	while(true){
	//for(int count=0;count<3;count++){
		Fake_algorithm();
		vector<int>::iterator practical_move = move_cell_name.begin();
		//cout<<"max_partial_sum: "<<max_partial_sum<<endl;
		if(max_partial_sum > 0){//真正的在CELL移動
			//更新 cell.set
			int local_cell;
			while((*practical_move) != max_partial_sum_cell_name){
				local_cell = (*practical_move);	while(cell[local_cell].cell_name != (*practical_move))	local_cell--;
				cell[local_cell].set = 3-cell[local_cell].set;
				cut_size -= cell[local_cell].gain;
				cell[local_cell].lock = false;
				practical_move++;
			}
			local_cell = (*practical_move);	while(cell[local_cell].cell_name != (*practical_move))	local_cell--;
			cell[local_cell].set = 3-cell[local_cell].set;
			cut_size -= cell[local_cell].gain;
			cell[local_cell].lock = false;
			practical_move++;
			while(practical_move != move_cell_name.end()){
				cell[local_cell].lock = false;
				practical_move++;
			}
			
			//--
			int cell_local;
			set_A.set_size = 0;	set_B.set_size = 0;	cut_size = 0;
			bool isCellInA, isCellInB;
			for(vector<NET>::iterator i=net.begin();i!=net.end();i++){
				(*i).distribution.first = 0;
				(*i).distribution.second = 0;
			}
			for(vector<NET>::iterator i=net.begin();i!=net.end();i++){//一條net
				isCellInA = false, isCellInB = false;
				for(vector<int>::iterator j=(*i).cell_name.begin();j!=(*i).cell_name.end();j++){//以net為主的cell
					cell_local = (*j)-1;	while(cell[cell_local].cell_name != (*j))	cell_local--;
					if(cell[cell_local].set == 1){
						isCellInA = true;
						(*i).distribution.first++;
					}else if(cell[cell_local].set == 2){
						isCellInB = true;
						(*i).distribution.second++;
					}else{
						cout<<"ERROR: set wrong!"<<endl;
					}
				}
				if(isCellInA && isCellInB)	cut_size++;
			}
			//--
			computing_cell_gain();
		}else{//set_A set_B 的cell_name
			set_A.cell_name.clear();
			set_B.cell_name.clear();
			for(vector<CELL>::iterator i=cell.begin();i!=cell.end();i++){
				if((*i).set == 1){
					set_A.cell_name.push_back((*i).cell_name);
				}else{
					set_B.cell_name.push_back((*i).cell_name);
				}
			}
			break;
		}
	}
} 
void computing_cell_gain(){//也建立bucket list 也讓cell.ptr連在bucket list上
	int check_A_cell, check_B_cell, gain_value_1, gain_value_2;
	set_A.set_size = 0,	set_B.set_size = 0;
	for(vector<CELL>::iterator i=cell.begin();i!=cell.end();i++){
		(*i).gain = 0;
	}
	for(vector<CELL>::iterator i=cell.begin();i!=cell.end();i++){	//針對每個cell去算gain
		if((*i).set == 1){//要看一下目前cell是在哪個set
			check_A_cell = 1;
			check_B_cell = 0;
			gain_value_1 = 1;//自己這邊剩自己 移過去得利
			gain_value_2 = -1;//對面的狀況 (原本無cut要變有 所以損失)
			set_A.set_size += (*i).cell_size;
		}else if((*i).set == 2){
			check_A_cell = 0;
			check_B_cell = 1;
			gain_value_1 = -1;//對面的狀況 (原本無cut要變有 所以損失)
			gain_value_2 = 1;//自己這邊剩自己 移過去得利
			set_B.set_size += (*i).cell_size;
		}else{	cout<<"error: cell no set."<<endl;	}
		//(*i).lock = false;//----------------------------------
		for(vector<int>::iterator j=(*i).net_name.begin();j!=(*i).net_name.end();j++){	//每次的連在該cell的所有net
			if(net[(*j)-1].distribution.first == check_A_cell){//看這個net的切割狀況 是否有兩邊有一邊為0或1	
				(*i).gain += gain_value_1;	//有的話要增減目前cell的gain值
			}
			if(net[(*j)-1].distribution.second == check_B_cell){
				(*i).gain += gain_value_2;
			}
		}
		//這裡就得到了此cell的gain了
		//建立bucket list
		pair<int, list<int> > temp_bucket_list;
		map<int, list<int> >::iterator iter;
		temp_bucket_list.first = (*i).gain;
		if((*i).set == 1){//建立A的bucket list
			iter = bucket_list_A.find((*i).gain);
			if(iter != bucket_list_A.end()){
				(*iter).second.push_back((*i).cell_name);
				(*i).ptr = --(*iter).second.end();
				//cout<<"new"<<*((*i).ptr)<<endl;
				//cout<<"gain_exist: "<<(*i).cell_name<<" "<<(*(*iter).cell_name.begin())<<" "<<(*iter).gain<<endl;
			}else{
				//cout<<(*i).cell_name<<" "<<(*i).gain<<endl;
				temp_bucket_list.second.push_back((*i).cell_name);
				bucket_list_A.insert(temp_bucket_list);
				iter = bucket_list_A.find((*i).gain);
				(*i).ptr = (*iter).second.begin();
			}
		}else{//建立B的bucket list
			iter = bucket_list_B.find((*i).gain);
			if(iter != bucket_list_B.end()){
				(*iter).second.push_back((*i).cell_name);
				(*i).ptr = --(*iter).second.end();
			}else{
				temp_bucket_list.second.push_back((*i).cell_name);
				bucket_list_B.insert(temp_bucket_list);
				iter = bucket_list_B.find((*i).gain);
				(*i).ptr = (*iter).second.begin();
			}
		}//建立bucket list完畢
	}
}
bool sort_net_nameStoB(NET a, NET b){//無問題
	return a.net_name < b.net_name;
}
void initial_partition2(){
	int cell_local;
	set_A.set_size = 0;	set_B.set_size = 0;	cut_size = 0;
	bool isCellInA, isCellInB;
	for(vector<NET>::iterator i=net.begin();i!=net.end();i++){//一條net
		isCellInA = isCellInB = false;
		if( (set_A.set_size+(*i).one_net_size-legal_size_range) <= (set_B.set_size) ){
			for(vector<int>::iterator j=(*i).cell_name.begin();j!=(*i).cell_name.end();j++){//以net為主的cell
				cell_local = (*j)-1;	while(cell[cell_local].cell_name != (*j))	cell_local--;
				if(cell[cell_local].set == 0){
					//全部放到A集合，除非有放過
					set_A.cell_name.push_back((*j));
					set_A.set_size += cell[cell_local].cell_size;
					cell[cell_local].set = 1;
					isCellInA = true;
					(*i).distribution.first++;
				}else{
					if(cell[cell_local].set == 1){
						isCellInA = true;
						(*i).distribution.first++;
					}else{
						isCellInB = true;
						(*i).distribution.second++;
					}
				}
			} 
		}else{
			for(vector<int>::iterator j=(*i).cell_name.begin();j!=(*i).cell_name.end();j++){//以net為主的cell
				cell_local = (*j)-1;	while(cell[cell_local].cell_name != (*j))	cell_local--;
				if(cell[cell_local].set == 0){
					//全部放到B集合，除非有放過
					set_B.cell_name.push_back((*j));
					set_B.set_size += cell[cell_local].cell_size;
					cell[cell_local].set = 2;
					isCellInB = true;
					(*i).distribution.second++;
				}else{
					if(cell[cell_local].set == 1){
						isCellInA = true;
						(*i).distribution.first++;
					}else{
						isCellInB = true;
						(*i).distribution.second++;
					}
				}
			}
		}
		if(isCellInA && isCellInB)	cut_size++;
	}
	sort(net.begin(),net.end(),sort_net_nameStoB);
}
void initial_partition(){
	int cell_local;
	set_A.set_size = 0;	set_B.set_size = 0;	cut_size = 0;
	bool isCellInA, isCellInB;
	vector<CELL>::iterator i=cell.begin(), j=cell.end();
	j--;
	while(i!=j){
		if(set_A.set_size <= set_B.set_size){
			(*j).set = 1;
			set_A.cell_name.push_back((*j).cell_name);
			set_A.set_size += (*j).cell_size;
			j--;
		}else{
			(*i).set = 2;
			set_B.cell_name.push_back((*i).cell_name);
			set_B.set_size += (*i).cell_size;
			i++;
		}
	}
	if(set_A.set_size <= set_B.set_size){
		(*i).set = 1;
		set_A.cell_name.push_back((*i).cell_name);
		set_A.set_size += (*i).cell_size;
	}else{
		(*i).set = 2;
		set_B.cell_name.push_back((*i).cell_name);
		set_B.set_size += (*i).cell_size;
	}
	
	for(vector<NET>::iterator i=net.begin();i!=net.end();i++){//一條net
		isCellInA = false, isCellInB = false;
		for(vector<int>::iterator j=(*i).cell_name.begin();j!=(*i).cell_name.end();j++){//以net為主的cell
			cell_local = (*j)-1;	while(cell[cell_local].cell_name != (*j))	cell_local--;
			if(cell[cell_local].set == 1){
				isCellInA = true;
				(*i).distribution.first++;
			}else if(cell[cell_local].set == 2){
				isCellInB = true;
				(*i).distribution.second++;
			}else{
				cout<<"ERROR: set wrong!"<<endl;
			}
		}
		if(isCellInA && isCellInB)	cut_size++;
	}
}
bool sort_cell_function(CELL a, CELL b){//無問題
	return a.cell_name < b.cell_name;
}
bool sort_net_function(NET a, NET b){//無問題
	return a.one_net_size > b.one_net_size;
}
void readCellSizeData(){//無問題
	char c;
	fstream fp;
	CELL cell_temp;
	legal_size_range = 0;
	fp.open(infilename_CELL, ios::in);//開啟檔案
    if(!fp){//如果開啟檔案失敗，fp為0；成功，fp為非0
        cout<<"Fail to open file: "<<infilename_CELL<<endl;
    }

	while(fp>>c){
		fp>>cell_temp.cell_name;
		fp>>cell_temp.cell_size;
		legal_size_range += cell_temp.cell_size;
		//cell_temp.net_name = 0;//--------
		cell_temp.set = 0;
		cell_temp.lock = false;
		cell_temp.gain = 0;
		cell.push_back(cell_temp);
	}
	//cout<<"Total cell size: "<<legal_size_range<<endl;
	legal_size_range /= 10;
	sort(cell.begin(), cell.end(), sort_cell_function);
	fp.close();//關閉檔案
}
void readNetData(){//無問題
	int temp;
	char c;
    fstream fp;
    NET net_temp;
	string str;
	int cell_local;

    fp.open(infilename_NET, ios::in);//開啟檔案
    if(!fp){//如果開啟檔案失敗，fp為0；成功，fp為非0
        cout<<"Fail to open file: "<<infilename_NET<<endl;
    }
	
	while(fp>>str){//for NET
		
		net_temp.cell_name.clear();
		net_temp.one_net_size = 0;
		net_temp.distribution.first=0;//還沒將net上的cell做分群
		net_temp.distribution.second=0;//還沒將net上的cell做分群		
		
		fp>>c;
        fp>>net_temp.net_name;//net number
		
		fp>>c;//左括號
		
		fp>>c;//c
		while(c != '}'){//temp_cell != "}\0"
			fp>>temp;//net_temp.cell_name
			cell_local = temp-1;	while(cell[cell_local].cell_name != temp)	cell_local--;
			cell[cell_local].net_name.push_back(net_temp.net_name);//多一個net到主要是cell中
			net_temp.one_net_size += cell[cell_local].cell_size;
			net_temp.cell_name.push_back(temp);
			fp>>c;
		}
		net.push_back(net_temp);
	}
	fp.close();//關閉檔案

}
void printUsage(string str){//無問題
    cerr << "Usage: " << str <<" [-n nets_file] [-c cells_file] \n"<<endl;
}
void parseCmd(int argc, char ** argv){// parse command//無問題
    if (argc < 2) {
        printUsage(argv[0]);
        exit(EXIT_FAILURE);
    }
    char opt = 0;
	//output_file = "abc";
    while ((opt = getopt(argc, argv, "n:c:o:")) != -1){
        switch (opt){
            case 'c':
                cout << "infilename_CELL = " << optarg << endl;
				infilename_CELL = optarg;
                break;
            case 'n':
                cout << "infilename_NET = " << optarg << endl;
				infilename_NET = optarg;				
				checkOutputName = optarg;
                break;
			case 'o':
                cout << "output_file = " << optarg << endl;
				output_file = optarg;
                break;
            case 'h' : default:
                printUsage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}
void write_solution_file(){//無問題
	fstream fpout;
	if(output_file !="0"){
		fpout.open(output_file, ios::out);
	}
	else if(checkOutputName == "testcases/p2-1.cells"){		
		fpout.open("output/FM_Partitioner_solution1", ios::out);
	}else if(checkOutputName == "testcases/p2-2.cells"){
		fpout.open("output/FM_Partitioner_solution2", ios::out);
	}else if(checkOutputName == "testcases/p2-3.cells"){
		fpout.open("output/FM_Partitioner_solution3", ios::out);
	}else{
		fpout.open("output/FM_Partitioner_solution", ios::out);
	}
	if(!fpout){
		cout<<"Fail to open file: "<<"solution"<<endl;
	}
	fpout<<"cut_size "<<cut_size<<endl;
	fpout<<"A "<<set_A.cell_name.size()<<endl;
	for(list<int>::iterator i=set_A.cell_name.begin();i!=set_A.cell_name.end();i++){
		fpout<<"c"<<(*i)<<endl;
	}
	fpout<<"B "<<set_B.cell_name.size()<<endl;
	for(list<int>::iterator i=set_B.cell_name.begin();i!=set_B.cell_name.end();i++){
		fpout<<"c"<<(*i)<<endl;
	}
	fpout.close();//關閉檔案
	cout<<"OK!"<<endl;
}
int main(int argc, char **argv){
    parseCmd(argc, argv);
	readCellSizeData();
	readNetData();
	initial_partition();
//----------------------------------------------------------------------------------------
	cout<<"Cell number:"<<cell.size()<<endl;
    cout<<"Legal size range: "<<legal_size_range<<endl;
	cout<<"Set_A area: "<<set_A.set_size<<endl;
	cout<<"Set_B area: "<<set_B.set_size<<endl;
	int area=0;
	for(vector<CELL>::iterator i=cell.begin();i!=cell.end();i++){
		area += (*i).cell_size;
	}
	cout<<"Cell area:"<<area<<endl;
	cout<<"cut_size: "<<cut_size<<endl;
	
//----------------------------------------------------------------------------------------
	computing_cell_gain();
	FM_algorithm();
	
	write_solution_file();
	cout<<"cut_size: "<<cut_size<<endl;
	return 0;
}
// 3/31新增了 1.net的pair(distribution)(一條net各有幾個cell在setA與setB) 2.算每個cell的gain值
// 4/3 219行可以用來加分用 //77行
