#include"streams.h"
#include"heap.h"
#include<iostream>
#include<string>
#include<vector>
#include<sstream>
#include<cstdlib>
#include<sys/types.h> // for ulong
#include<queue>
#include<fstream> // FOR READING final output sorted file because i am lazy
#include<time.h>

int get_file_index(std::string &s);
std::string make_file_name(int i);
std::string make_out_file_name(int i);
int d_way_merge(std::vector<IStreamReadBuf> &rbs_vector,std::vector<int> &infiles,int d ,OStreamWriteBuf &outfiles);
int compare (const void * a, const void * b);

const bool DEBUG= false ;
#define HIGH  99999 ;
using namespace std ;
int main(int argc, char *argv[]) {

	time_t seconds;
	/*
	   Get value from system clock and
	   place in seconds variable.
	   */
	time(&seconds);
	/*
	   Convert seconds to a unsigned
	   integer.
	   */
	srand((unsigned int) seconds);
	if(argc != 5){
		cout  << "please enter the n , m ,d,b " << endl ;
		return 1; 
	}	
	int pagesize = getpagesize() ;
	// USER INPUT IS MULTIPLE OF PAGE SIZE 
	int  n  = atoi(argv[1]);
	int  m  = atoi(argv[2]);
	const int d = atoi(argv[3]);
    if(((n*4)/m) < d) {
		cout <<"the n/m values is less than d trying "<< endl ;
		if(n/m < 2){
		cout << "the n/m values is less than or equal to  1.. exiting" <<endl ;
			return 1;	
		}
	}	
	const int B = atoi(argv[4]) ; 
	const ulong N = (ulong)n * pagesize ;
	const ulong M = (ulong)m * pagesize ;
	
	ulong offset = 0; 
	// create master file 
	OStreamWriteBuf writebuf_stream(B); // buffer size = optimum  
	string s = "m_files/master.bin";
	writebuf_stream.create(s);
	//for(ulong i=0; i < N/4 ; i++) // 10*pagesize elements written ==> size of file = N  
	for(ulong i=0; i < N ; i++) // 10*pagesize elements written ==> size of file = N  
	{	
		int num = rand() % HIGH + 1 ;
		//writebuf_stream.writes(i);
		writebuf_stream.writes(num);

	}
	writebuf_stream.closes();
	// START READING THE LARGE FILE and divide it into N/M streams 
	//int buffer_size = B ;
	// we have to make N/M streams use a vector 
	vector<IStreamReadBuf>  rb_vector(N/M,B);
	// now loop thru this vector and opens master.bin at different offsets 
	for(ulong i=0 ; i < N/M ; i++){
			rb_vector[i].opens(s,offset);
			offset = offset + M ; 
	}
	// NOW write read these streams one at a time and store them in an array 
	int temp ;
	for(int i = 0; i < N/M ; i++){ // loop thru all the streams 
			 // temp_array
			 int *temp_array = new int[M/sizeof(int)] ;
			 //read M elements 
			 for(int j=0; j< (M/sizeof(int)) ;j++ ){
					temp = rb_vector[i].read_next() ;
					if(temp == -1 || temp == -2 ){
					    cout << "read next failed or you generated random number"<<endl ;
						return 1 ; // main return 
					}	
				 	temp_array[j] = temp ;
			 }
			 // sort this array using quick sort 
			 qsort(temp_array,(M/sizeof(int)),sizeof(int),compare);
			 // now write these M / sizeof(int) elements to a file

			 string filename = make_file_name(i) ;
			 // open file 
			 //cout << "creating filename " << filename << endl ;
			 OStreamWriteBuf wb(B) ; // CHANGE THE BUFFER SIZE TO OPTIMAL
			 wb.create(filename);
			 // ELEMENTS SHOULD BE WRITTEN IN DESCENDING ORDER 
			 for(int j=(M/sizeof(int) - 1 ) ; j >= 0 ; j--){
						wb.writes(temp_array[j]);		
			 }
			 wb.closes();
			 delete [] temp_array ;

	}	

	// close the open streams 
	for(int i = 0; i < N/M ; i++){
			rb_vector[i].closes();
	}	
	// NOW Open these streams for reading use queue STL to store the istream references  
	//queue<IStreamReadBuf *>     rbs_queue;
	queue<int>     rbs_queue;
	vector<IStreamReadBuf>  rbs_vector(N/M,B);
		// open the new streams 
		for(int i =0; i < N/M ; i++){
			
			 string filename = make_file_name(i) ;
			 rbs_vector[i].opens(filename);
			 // store references of the streams in the queue  
			 rbs_queue.push(i);
		}
	
	// PART 3
	 int vector_size = N/M ;	
	 int out_index = 0 ;	
	 int ret_code = 0;
	 int new_index = 0 ;
 	 //IStreamReadBuf **infiles = new IStreamReadBuf*[d]; //or  vector<IStreamReadBuf *> var(d)
	 vector<int> infiles(d) ;
	 int elements_popped ;
	 std::string last_file_name ; 
	 // while elements remain pop d elements from the queue then store them in  
	 while(rbs_queue.size() != 1){
			
		 	// pop d elements 
			elements_popped = 0 ;		
			for(int i = 0 ; i < d ; i++){
					if( !rbs_queue.empty() ){
						infiles[i] = rbs_queue.front();
            			rbs_queue.pop();
						elements_popped += 1 ;
					}
					else{
						break ;
					}	
			}
			OStreamWriteBuf outfile(B) ; // CHANGE BUFFER SIZE TODO
			new_index = vector_size + out_index ;
			std::string out_file_name = make_out_file_name(new_index);
			last_file_name = out_file_name ;
			outfile.create(out_file_name);
			// d_way_merge starts for these d files 
			int d_ret = d_way_merge(rbs_vector,infiles,elements_popped,outfile);
			// DELETE THE MERGED FILES 
			for(int i=0;i<elements_popped;i++ ){
	 				std::string rm_cmd = "rm ";
					string filename = rbs_vector[infiles[i]].getFilename();
					//int ret = rbs_vector[infiles[i]].closes() ;
					rm_cmd.append(filename);
					//cout << " deleting merged file =="<< filename << endl ;
					int ret = system(rm_cmd.c_str());
					if(ret != 0){
						cout << "could not delete the merged file number"<<infiles[i] <<endl ;
					}	
					
			}	
			//cout <<"ret_code for d_way_merge "<< d_ret << endl ;
			// close output file 
			int ret_code = outfile.closes();
			if(ret_code < 0){
					cout << "ret code for closes =" << ret_code <<" breaking for while loop " << endl ;
					break ;
			}
			// open a d merge object and add it to the queue 
			IStreamReadBuf temp_read(B) ;  // TODO Add Buffer size
			ret_code = temp_read.opens(out_file_name);
			//cout << "opened file name " << out_file_name ;
			if(ret_code < 0){
					cout << "could not opene file " << out_file_name << endl ;
					cout << "ret code for opens = " << ret_code <<" breaking for while loop " << endl ;
					break ;
			}	
			rbs_vector.push_back(temp_read); // index of new object = vector_size + out_index + 1
			int new_index = vector_size + out_index  ;
			//cout << "new index =  "<< new_index <<endl ;
			rbs_queue.push(new_index);
		//	cout << "sanity check temp_object filename == " << temp_read.getFilename() << "is same as vector filename == " << rbs_vector[new_index].getFilename() << endl ;
			

			out_index += 1 ;
			//cout << "end of iteration " << endl ;
			//delete [];i
			//for(int i = 0 ; i < d ; i++){
					//cout << "deleting " << i ;
					//delete infiles[i];
			//}
			//delete [] infiles ;
			//cout << "end of iteration 2 " << endl ;
			//infiles = NULL ;
								             
    }
	// read the last file 
	//outfile.close();
	// READ THE OUTPUT FILE
////////// ******* READING BACK OUR OUTPUT 	 
/*ifstream read_out ;
    read_out.open(last_file_name.c_str(),ios::binary);
    if(read_out.fail()){
		       cout << "open failed " << endl ;
				            return 1 ;
    }
    while(!read_out.eof() ) {
	         int num ;
	         read_out.read((char*)&num,4);
			 if(read_out.eof()){
				continue ;
			}

		cout << num << endl ;
  }
  read_out.close();
*/

	cout << "PROGRAM EXECUTED SUCCCESSFULLY " << endl ;	
}

int d_way_merge(vector<IStreamReadBuf> &rbs_vector,vector<int> &infiles,int d,OStreamWriteBuf & outfile){

		// initialize the heap
	    PQ  heap(d);
		int streams_open = d ;
		//cout << "inside d_way _merge " << endl ;
		// write the first d elements 
		for(int j= 0 ; j < d ; j++){
			//int number ;
			//string filename = infiles[j]->getFilename() ; 
			string filename = (rbs_vector[infiles[j]]).getFilename() ; 
			//cout << " filename of new elemetn inserted into heap"<< filename  << endl ;
			int number = rbs_vector[infiles[j]].read_next() ;
			Item a(number,filename);
			heap.insert(a) ;
			//cout << "read and inserted number " << number << endl ; 
		}
		
		while(streams_open > 0 ){


			Item d = heap.getmax();
			int n = d.getNumber() ;
			string s = d.getFile();
			//cout << "deleted number " << n << " from heap from file " << s<<endl ;
			string filename = s ; // calls a copy constructor i
			// WRITE THE MAX element to file 
			int ret_code = outfile.writes(n);
			if(ret_code < 0){
					cout << "writing to output failed breaking from while loop "<<endl ;
					break ; 
			}
			// split s to find out the corresponding open stream
			int file_index = get_file_index(s);
			if(file_index == -1 ){
					cout << "invalid file index exiting "<< endl ;
					return -1 ;
			}			
			//cout << "file index of file we are reading from ==" << file_index << endl ;
			if(rbs_vector[file_index].end_of_stream()){
					streams_open-- ;	
					ret_code = rbs_vector[file_index].closes();
					if(DEBUG){	
					cout << "****File " << filename << "reached eof closing corresponding stream " << endl ;
					}
					// WE wont insert here
					continue ;
			}
			//cout << "here " <<endl ;
			// read element from the file that got removed from heap  
			//int number ;
			int number = rbs_vector[file_index].read_next();
			Item a(number,filename);
			// now add this number to heap i
			//cout <<"adding  number to the heap = "<< number <<"got this number from file  " << filename << endl ;
			heap.insert(a);

			// Always check for eof after READ !!! IMP 
			
	}
	//cout << "reached  end of d_way merge " << endl ;
	return 0 ;

}	
std::string  make_file_name(int i){

		std::string filename("");
		std::ostringstream sin ;
		const std::string prefix = "m_files/slave_" ;
		const std::string suffix = ".bin";
		sin << i ;
    	filename.append(prefix);
    	filename.append(sin.str());
    	filename.append(suffix);
		sin.str("");
		return filename ;

}
std::string  make_out_file_name(int i){

		std::string filename("");
		std::ostringstream sin ;
		const std::string prefix = "m_files/d_merged_" ;
		const std::string suffix = ".bin";
		sin << i ;
    	filename.append(prefix);
    	filename.append(sin.str());
    	filename.append(suffix);
		sin.str("");
		return filename ;

}
int get_file_index(std::string &s){

			size_t index = s.find(".");
			if(index == std::string::npos){
				std::cout << " dot character not found wtf " << std::endl ;
					return -1 ;
			}
			
			//cout << "index of dot character "<< index <<endl ;
			s.erase(index);
			index = s.rfind("_") + 1 ;
			s.erase(0,index);
			//cout << " string after erase " << s << endl ;
			int file_index = atoi(s.c_str())	;
			return file_index ;

}
int compare (const void * a, const void * b)
{
	  return ( *(int*)a - *(int*)b );
}
