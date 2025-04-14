#include "buffer_mgr.h"
#include "disk.h"

#include <ctime>
#include <set>

extern int readCount,writeCount;

#define BUFFER_SIZE 10


int main(){

    srand(time(NULL));
    
    string f1="stud_sport";
    string f2="stud_city";

    //2 relations:
    //1. R1 : (student roll no. -> char (9),sport -> varchar (21))
    //2. R2 : (student roll no. -> char (9),city -> varchar(21))
    
    int n1=1000;
    int n2=1000;

    //create files f1 , f2 with n1 , n2 number of records

    FILE* fptr1=fopen("stud_sport","w");
    FILE* fptr2=fopen("stud_city","w");

    set<pair<int,int>> st;


    int num=7;
    char * sports[]={strdup("BasketBall"),strdup("Cricket"),strdup("Baseball"),strdup("Tennis"),strdup("Badminton"),strdup("TableTennis"),strdup("Swimming")};
    char * city[]={strdup("Kharagpur"),strdup("Patiala"),strdup("Sangrur"),strdup("Nabha"),strdup("Ludhiana"),strdup("Amritsar"),strdup("Gurdaspur")};

    for(int i=0;i<n1;i++){
        int yr=rand()%13+11;
        int roll=rand()%10000+10000;

        //Fill in here
        if(st.find({yr,roll})==st.end()){
            int id1,id2;
            id1=rand()%7;
            id2=rand()%7;
            fprintf(fptr1,"%dCS%d,%21s\n",yr,roll,sports[id1]);
            fprintf(fptr2,"%dCS%d,%21s\n",yr,roll,city[id2]);
            st.insert({yr,roll});
        }

    }



    int record_size=32; //2 extra bytes for , (between the 2 attributes) and \n (after every record)

    BM_PageHandle bm1,bm2;
    BufferPool bm_join(BUFFER_SIZE,RS_LRU);

    int num_records=(PAGE_SIZE/record_size);

    for(int r1=0;r1<n1;r1++){
        int p1=(r1*record_size)/PAGE_SIZE;
        bm1.pageNum=p1;
    
        if(r1%num_records==0)bm_join.pinPage(bm1,p1,f1);

        char * data1 = bm1.data;

        for(int r2=0;r2<n2;r2++){
            int p2=(r2*record_size)/PAGE_SIZE;

            bm2.pageNum=p2;
            if(r2%num_records==0)bm_join.pinPage(bm2,p2,f2);
            
            char * data2=bm2.data;

            // use data1 data2 to get the actual data and calculate the join
            
            if((r2+1)%num_records==0 || r2+1==n2)bm_join.unpinPage(p2,f2);
        }

        if((r1+1)%num_records==0 || r1+1==n1)bm_join.unpinPage(p1,f1);
    }

    cout<<"Disk reads "<<readCount<<"\n";
    cout<<"Disk writes "<<writeCount<<"\n";

}