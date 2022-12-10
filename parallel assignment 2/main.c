#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define bool int
#define true 1
#define false 0
#define lineMaxSize 1000
#define fileNameSize 100
#define numberOfFiles 50

char folderName[]="Aristo-Mini-Corpus";

// this function takes the id of the file and returns its name
void getFileName(char res[],int id){
    sprintf(res, "%s/%s %s%d%s",folderName,folderName,"P-", id,".txt");
}

// this function takes a line and a string and searches for that string in this
// line and returns true if it was found
bool contains(char line[],char query[]){
    int sz = strlen(line);

    // -1 to ignore the \n
    int sz2=strlen(query)-1;

    char words[100][100];
    int curWord=0,curInd=0;
    for(int i=0 ; i<sz2 ;i++){
        if(query[i]==' '){
            words[curWord++][curInd]='\0';
            curInd=0;
        }else{
            words[curWord][curInd++] = query[i];
        }
    }
    words[curWord++][curInd]='\0';
    bool res=true;
    for(int word=0 ; word<curWord ; word++){
        bool wordFound=false;
        sz2=strlen(words[word]);
        for(int i=0 ; i<sz-sz2+1 ; i++){
            bool found=true;
            for(int j=0 ; j<sz2 ; j++){
                if(words[word][j] != line[i+j]){
                    found=false;
                    break;
                }
            }
            if(found){
                wordFound=true;
                break;
            }
        }
        res&=wordFound;
    }
    return res;
}
char fileName[fileNameSize];
char line[lineMaxSize];
void getData(char query[],int id,FILE *out){
     getFileName(fileName,id);
     FILE *fp=fopen(fileName, "r");
     while(fgets(line, lineMaxSize, fp)){
         if(contains(line,query)){
             fprintf(out,"%s",line);
         }
     }
     fclose(fp);
}

int main(int argc, char* argv[]){
    int numOfProcesses;			/* number of process	*/
    int my_rank;		/* rank of process	*/

    MPI_Init( &argc , &argv );
    double st=MPI_Wtime();
    /* Find out.out process rank */
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numOfProcesses);
    int numOfFilesForEachProcess[numOfProcesses];
    int startingFileEachProcess[numOfProcesses];
    char query[lineMaxSize];
    if(my_rank==0){
        int base=numberOfFiles / numOfProcesses;
        for(int i=0 ; i<numOfProcesses ; i++){
            numOfFilesForEachProcess[i] = base;
        }
        // the number of files that will take additional file
        int addition = numberOfFiles%numOfProcesses;
        for(int i=0 ; i<addition ; i++){
            numOfFilesForEachProcess[i]++;
        }
        int curStartFile=1;
        for(int i=0; i<numOfProcesses ; i++){
            startingFileEachProcess[i]=curStartFile;
            curStartFile+=numOfFilesForEachProcess[i];
        }
        printf("Enter your query\n");
        fgets(query,lineMaxSize,stdin);
    }
    MPI_Bcast(query,lineMaxSize,MPI_CHAR,0,MPI_COMM_WORLD);
    int numOfFilesForThisProcess,startingFileForThisProcess;

    MPI_Scatter(&numOfFilesForEachProcess,1,MPI_INT,&numOfFilesForThisProcess
            ,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Scatter(&startingFileEachProcess,1,MPI_INT,&startingFileForThisProcess
            ,1,MPI_INT,0,MPI_COMM_WORLD);

    // now each process knows the files it should search in

    if(my_rank==0){
        sprintf(fileName,"res.txt");
    }else{
        sprintf(fileName,"%d.txt",my_rank);
    }
    FILE *fp=fopen(fileName, "w");
    int endFile = startingFileForThisProcess + numOfFilesForThisProcess-1;
    for(int fileId=startingFileForThisProcess ; fileId<=endFile ; fileId++){
        getData(query,fileId,fp);
    }
    // wait till all the process finish all their work
    MPI_Barrier(MPI_COMM_WORLD);

    if(my_rank==0){
        for(int i=1 ; i<numOfProcesses ; i++) {
            sprintf(fileName,"%d.txt",i);
            FILE *in = fopen(fileName, "r");
            while (fgets(line, lineMaxSize, in)) {
                fprintf(fp, "%s", line);
            }
            fclose(in);
        }
        fclose(fp);
        double en=MPI_Wtime();
        printf("the program takes %f ms\n",en-st);
    }else{
        fclose(fp);
    }
    MPI_Finalize();
    return 0;
}