#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <cmath>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <cstring>
#include <queue>
#include <stdlib.h>
#include <algorithm>
#include <math.h>
using namespace std;


/** 
 *
 * FILE: merge_triangulated_tracks.cpp
 * Authors: Krishna Kumar Singh and Aditya Deshpande
 * Usage: ./a.out <triangulated tracks dir>
 *
 * This code merges the triangulated tracks to get
 * reduced set of consistent tracks.
 *
 **/


map < unsigned long long int, vector <unsigned long long int> > my_map ;
map < unsigned long long int, vector <int> > my_track ;
map < unsigned long long int, int > size_vec;
map < unsigned long long int, double> x_cord;
map < unsigned long long int, double> y_cord;

map<unsigned long long int, vector <unsigned long long int> >::iterator iter;
map < unsigned long long int, int > visited;
vector < vector < unsigned long long int> > track;
vector < vector <int> > parent_track;

int g_cnt=0;

int main(int argc, char* argv[]) {

    if(argc!=3) {
        printf("[USAGE] ./merge_tracks <triangulationDirPath> <outputPath>\n");
        return 1;
    }

    char results_dir[1000], output_dir[1000];
    strcpy(results_dir, argv[2]);
    strcpy(output_dir, argv[3]);


    printf("[DEBUG] Results Directory : %s\n", results_dir);

    char command[1000];
    sprintf(command,"ls %s/input-tracks.txt", results_dir);
    FILE *fSys = popen(command, "r");

    char path[1000];

    int num;
    double rError;

    printf("\nPath for searching tracks %s", command);
    fflush(stdout);

    while(fgets(path, sizeof(path)-1, fSys) != NULL) {
        path[strlen(path)-1] = '\0';
        printf("[DEBUG] Reading Output File: %s\n", path);	
        FILE *f = fopen(path, "r");
        char line[1000];
        char waste[1000];
        char newLineChar;
        int lineCtr = 0;
        while(fscanf(f, "%d",&num)!=EOF) {
            lineCtr++;
            if(lineCtr%1000 == 0) {
                printf("\nReading file %s, line %d", path, lineCtr);
            }
            g_cnt++;
            unsigned long long int ar[num];
            if(ar == NULL) {
                printf("\nCould not allocate memory");
            }

            for(int i=0;i<num;i++) {
                ar[i]=0;
                long long int im_id;
                long long int sift_id;
                fscanf(f,"%lld",&im_id);
                fscanf(f,"%lld",&sift_id);

                // View ID and Sift ID are packed to a single long int
                ar[i]|= im_id << 32;
                ar[i]|= sift_id;
            }

            // O(N*N) loop to create adjacency list
            for(int i=0;i<num;i++) {

                vector< unsigned long long int> temp;
                int size;

                if( my_map.find(ar[i]) != my_map.end()) {
                    temp = my_map[ar[i]];
                    size = size_vec[ar[i]];
                    int cnt=size;
                    for (int j=0;j<num;j++) {
                        if(cnt >= temp.size())
                        {
                            temp.resize(2*temp.size());
                        }
                        temp[cnt]=ar[j];
                        cnt++;		
                    }
                    my_map[ar[i]]=temp;
                    size_vec[ar[i]]=cnt;
                    my_track[ar[i]].push_back(g_cnt);
                }
                else {
                    if(num<100) {
                        temp.resize(100);
                    }
                    else {
                        temp.resize(num);
                    }
                    for (int j=0;j<num;j++)
                    {
                        temp[j]=ar[j];
                    }
                    size_vec[ar[i]]= num;
                    my_map[ar[i]]= temp;
                    vector <int> track_rec;
                    track_rec.push_back(g_cnt);
                    my_track[ar[i]]= track_rec;
                }

            }
        }
        fclose(f);
    }
    printf("[DEBUG] g_cnt %d\n", g_cnt);


    parent_track.resize(100000000);

    track.resize(100000000);

    int track_cnt=0;

    printf("[DEBUG] Initializaing visited vector\n");
    // Initialize the visited vector and resize my_map
    for(iter=my_map.begin();iter!=my_map.end();++iter) {

        vector< unsigned long long int> temp;
        temp = my_map[iter->first];
        temp.resize(size_vec[iter->first]);
        my_map[iter->first]=temp;
        visited[iter->first]=0;
    }

    printf("[DEBUG] Finding connected components\n");
    for(iter=my_map.begin();iter!=my_map.end();++iter) {

        if(visited[iter->first]==0) {

            queue<unsigned long long int> my_queue;
            vector <unsigned long long int> temp_track;
            vector <int> remove_views;
            temp_track.resize(100);
            int cnt=0;
            my_queue.push(iter->first);
            visited[iter->first]=1;
            int parent_cnt=0;

            while(!my_queue.empty()) {

                unsigned long long int temp = my_queue.front();
                if(cnt>= temp_track.size()) {
                    temp_track.resize(temp_track.size()*2);
                }
                int flag=0;

                vector<int>::iterator itr = 
                    find( remove_views.begin(), remove_views.end(), (int)(0|(temp>>32)));
                if(itr != remove_views.end()) {
                    flag = 1;
                } else { 
                    for(int j=0; j<cnt;j++) {
                        unsigned long long int tempu = temp_track[j];
                        if((0|(temp >> 32)) == (0|(tempu >> 32))) {
                            temp_track.erase(temp_track.begin()+j);
                            cnt--;
                            flag=1;
                            remove_views.push_back((int)(0|(temp >> 32)));
                            break;
                        }
                    }
                }

                if(flag==0) {
                    temp_track[cnt]=temp;
                    if(parent_cnt==0) {
                        parent_track[track_cnt]=my_track[temp];
                        parent_cnt = my_track[temp].size();
                    } else {
                        if(my_track[temp].size()>1)
                        {
                            for(int l=0;l<my_track[temp].size();l++)
                            {
                                parent_track[track_cnt].push_back(my_track[temp][l]);
                            }
                        }
                    }
                    cnt++;
                }	
                my_queue.pop();
                vector< unsigned long long int> temp_vec;
                temp_vec = my_map[temp];
                vector<unsigned long long int>::iterator iter1;
                for(iter1=temp_vec.begin();iter1!=temp_vec.end();++iter1) {
                    if(visited[*iter1]==0) {
                        my_queue.push(*iter1);
                        visited[*iter1]=1;
                    }
                }
            }

            sort(parent_track[track_cnt].begin(),  parent_track[track_cnt].end());

            parent_track[track_cnt].erase( unique(  
                        parent_track[track_cnt].begin(),  
                        parent_track[track_cnt].end() ),  
                    parent_track[track_cnt].end() );

            temp_track.resize(cnt);
            track[track_cnt]= temp_track;
            track_cnt++;
        }

    }
    printf("[DEBUG] Done finding connected components, tracks found %d\n", track_cnt);

    track.resize(track_cnt);
    parent_track.resize(track_cnt);
    vector< vector<unsigned long long int> >::iterator iter2;

    char tracksPath[1000];
    sprintf(tracksPath,"%s/merged-tracks.txt",output_dir);
    printf("\nWriting all results to file %s", tracksPath);
    fflush(stdout);

    FILE *ftrack = fopen(tracksPath,"w");
    fprintf(ftrack, "%d\n", track_cnt);
    for(iter2=track.begin(); iter2!=track.end(); ++iter2) {
        vector <unsigned long long int> my_temp;
        my_temp = *iter2;
        int tsize= my_temp.size();
        if(tsize>1) {
            vector<unsigned long long int>::iterator iter1;
            int i = 0;
            for(iter1=my_temp.begin();iter1!=my_temp.end();++iter1) {
                unsigned long long int tempu = *iter1;
                unsigned long long int view = (0|(tempu >> 32));
                unsigned long long int one = 1;
                unsigned long long int siftid = (tempu & ((one<<32)-1));
                i++;}
            vector< int > pt_views;
            for(iter1=my_temp.begin();iter1!=my_temp.end();++iter1) {
                unsigned long long int tempu = *iter1;
                pt_views.push_back((int)(0|(tempu >> 32)));}
            if(tsize > 1) { 
                int j = 0;
                fprintf(ftrack, "%d ", tsize);
                for(iter1=my_temp.begin();iter1!=my_temp.end();++iter1) {
                    unsigned long long int tempu = *iter1;
                    unsigned long long int view = (0|(tempu >> 32));
                    unsigned long long int one = 1;
                    unsigned long long int siftid = (tempu & ((one<<32)-1));
                    fprintf(ftrack, "%d %d ", view, siftid);
                    j++;
                }
                fprintf(ftrack,"\n");} 
        }
    }
    return 0;
}
