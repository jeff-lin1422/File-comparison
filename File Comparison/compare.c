#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "strbuf.c"
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include <pthread.h>
#include <math.h>

#define STRING_MAX 1000




typedef struct Queue{
  char name[STRING_MAX];
  struct Queue* next;
}Queue;

typedef struct wordList{
  char* word;
  int occurrence;
  double frequency;
  //int total;
  struct wordList* next;
}wordList;

typedef struct wfdRepo{
  char fileName[STRING_MAX];
  int total;
  wordList* wfd;
  struct wfdRepo* next;
}wfdRepo;

typedef struct threadArgs{
  Queue** files;
  Queue** directories;
  char* suffix;
  wfdRepo* repo;
  int dirthreadAmt;
  int filethreadAmt;
}threadArgs;



typedef struct repoQueue {
  wfdRepo* repo1;
  wfdRepo* repo2;
  struct repoQueue* next;
}repoQueue;

typedef struct JSDList {
  char fileName1[STRING_MAX];
  char fileName2[STRING_MAX];
  double JSDval;
  int totalOcc;
  struct JSDList* next;
}JSDList;

typedef struct anThreadArgs{
  repoQueue** rq;
 JSDList* jl;
}anThreadArgs;

pthread_mutex_t colLock;
pthread_mutex_t anLock;
pthread_cond_t dirSig;
pthread_cond_t fileSig;


//--------------------------------PROTOTYPES-------------------------------
//TODO: make all the prototypes
void checkDirectory(char* , Queue* , Queue* , char* );
void printQueue(Queue*);

//--------------------------------QUEUE STUFF-------------------------------

void init_queue(Queue* file){
  //assume file was malloced
  file->next = NULL;
  file->name[0] = '\0';
}

//takes in the name of the file/directory, then create a new node and add it to the end of the queue
void enqueue(Queue* q, char* item){
  if(q->name[0] == '\0')
  {
    memcpy(q->name, item, strlen(item)+1);
    return;
  }
  while(q->next != NULL){
    q = q->next;
  }
  Queue* newNode = malloc(sizeof(Queue));
  init_queue(newNode);
  memcpy(newNode->name, item, strlen(item)+1);
  q->next = newNode;
}

Queue* dequeue(Queue* q){
  Queue* temp = q;
  if(temp == NULL) {
    return q;
  }
  if(temp->name[0] == '\0') return q;
  q = q->next;
  free(temp);
  return q;

}

void printQueue(Queue* q){
  Queue* temp = q;
  while(temp != NULL){
    printf("%s-->", temp->name);
    temp = temp->next;

  }
  printf("NULL\n");
}

//free the memory from Queue
void destroyQueue(Queue* q){
  if(q->next != NULL){
    destroyQueue(q->next);
  }
  free(q);
}

//--------------------------------WORDLIST STUFF-------------------------------
void destroywordList(wordList* f){
  if(f->next !=NULL) destroywordList(f->next);
    free(f->word);
  free(f);
  return;
}


void insertwordList(wordList* wfdlist, char* word){
  wordList* temp = wfdlist;
  //check if theres anything in the hashtable first, if not add to it
  if(temp->word[0] == '\0')
  {
    free(temp->word);
    temp->word = word;
    temp->occurrence++;
    return;
  }
  while(temp !=NULL && strcmp(temp->word, word)!= 0)
  {
    temp = temp->next;
  }
  if(temp == NULL) {
    //add to it
    temp = malloc(sizeof(wordList));
    temp->word = word;
    temp->occurrence = 1;
    temp->next = NULL;

    wordList* last = wfdlist;
    while(last->next!=NULL) last=last->next;
    last->next = temp;
    return;
  }
  free(word);
  temp->occurrence++;
}

void init_wordList(wordList* l){
    //assumes l is already malloced
    l->word = calloc(2, sizeof(char));
    l->word[0] = '\0';
    l->occurrence = 0;
    l->next = NULL;
    l->frequency = 0.0;
}

void printwordList(wordList* wfdlist){
    wordList* temp = wfdlist;
    while(temp != NULL){
      printf("%s / %d / %f--->", temp->word, temp->occurrence, temp->frequency);
      temp = temp->next;
    }
    printf("\n");
}

void printwordListfreq(wordList* wfdlist){
    wordList* temp = wfdlist;
    while(temp != NULL){
      printf("%s / %f--->", temp->word,  temp->frequency);
      temp = temp->next;
    }
    printf("\n");
}
bool lookUpwordList(wordList* wfdlist, char* word){
  wordList* temp = wfdlist;
  while(temp !=NULL && strcmp(temp->word, word)!= 0)
  {
    temp = temp->next;
  }
  if(temp == NULL) return false;
  return true;
}

void copy_wordList(wordList* dest, wordList* src){
  //assumes dest has already been malloced
  wordList* tempdest = dest;
  wordList* tempsrc = src;
  if(src == NULL|| dest == NULL){
    return;
  }
  while (tempsrc!=NULL)
  {
    if(tempsrc->next !=NULL){
      tempdest->next = malloc(sizeof(wordList));
      init_wordList(tempdest->next);
    }
    free(tempdest->word);
    tempdest->word = malloc(sizeof(char)* strlen(tempsrc->word)+2);
    memcpy(tempdest->word, tempsrc->word, strlen(tempsrc->word)+1);
    tempdest->frequency = tempsrc->frequency;
    tempdest= tempdest->next;
    tempsrc = tempsrc->next;
  }

}
//--------------------------------WFDREPO STUFF-------------------------------

void init_wfdRepo(wfdRepo* repo){
  repo->fileName[0] = '\0';
  repo->total = 0;
  repo->next = NULL;
  repo->wfd = malloc(sizeof(wordList));
  init_wordList(repo->wfd);
}

void destroywfdRepo(wfdRepo* repo){
  // TODO fix this
  if(repo->next != NULL){
    destroywfdRepo(repo->next);
  }
  destroywordList(repo->wfd);
  free(repo);
  return;
}

void printwfdRepo(wfdRepo*l ){

  printf("_____________________________________________________\n");
  printf("PRINTING REPOSITORY....\n");
  wfdRepo* temp = l;
  while(temp!=NULL)
  {
    printf("\n------------------------\n PRINTING %s :\n", temp->fileName);
    printf("TOTAL: %d\n", temp->total);
    printwordList(temp->wfd);
    temp = temp->next;
  }
  printf("_____________________________________________________\n\n");
}

void setTotal_wfdRepo(wfdRepo* repo){
  wfdRepo* tempRepo = repo;
  while(tempRepo!=NULL)
  {
    wordList* tempwl = tempRepo->wfd;
    while(tempwl !=NULL)
    {
      tempRepo->total += tempwl->occurrence;
      tempwl = tempwl->next;
    }
    tempRepo = tempRepo->next;
  }
}

void setFrequencies(wfdRepo* repo){
  wfdRepo* tempRepo = repo;
  while(tempRepo != NULL){
    wordList* list = tempRepo->wfd;
    int total = tempRepo->total;
    //printf("setting frequency for %s\n", repo->fileName);
    while(list != NULL){
      //printf("occurrence/total =  %d / %d\n", list->occurrence, total);
      list->frequency = (double)(list->occurrence)/(double)(total);
      list = list->next;
    }
    tempRepo = tempRepo->next;
  }
}

//--------------------------------ANALYSIS STUFF-------------------------------
double getFrequency(wordList* f, char* word){
  wordList* temp = f;
  while(strcmp(temp->word, word)!= 0)
  {
    temp = temp->next;
  }
  return temp->frequency;
}

wordList* addToListWithFrequency(wordList* f, char* name, double frequency){
  if(f == NULL){
    f = malloc(sizeof(wordList));
    init_wordList(f);
    free(f->word);
    f->word = malloc(sizeof(char)*(strlen(name)+1));
    memcpy(f->word , name, (strlen(name)+1));
    f->frequency = frequency;
  }
  else{
    wordList* temp = f;
    while(temp->next != NULL){

      temp = temp->next;
    }
    wordList* newNode = malloc(sizeof(wordList));
    init_wordList(newNode);
    free(newNode->word);
    newNode->word = malloc(sizeof(char)*(strlen(name)+1));
    memcpy(newNode->word , name, (strlen(name)+1));
    newNode->frequency = frequency;
    temp->next = newNode;

  }
return f;
}

wordList* getMeanFreqlist(wordList* f1, wordList* f2){
  // set the default as f1, then we either add f2 if not found in 1, or add up frequencies
  wordList* returnList = malloc(sizeof(wordList));
  init_wordList(returnList);
  copy_wordList(returnList, f1);
  wordList* temp2 = f2;
    // add words in f2 is not found in f1
  while(temp2 != NULL){
    // if not found, do something
    if(lookUpwordList(f1, temp2->word) == false){
      addToListWithFrequency(returnList, temp2->word, temp2->frequency);
    }
    temp2 = temp2->next;
  }
  wordList* tempreturnList = returnList;
  while(tempreturnList != NULL){
    // case: when a word in f1 is found or unfound in f2
    if(lookUpwordList(f2, tempreturnList->word) == true && lookUpwordList(f1, tempreturnList->word) == true){
      tempreturnList->frequency += getFrequency(f2, tempreturnList->word);
    }
    tempreturnList->frequency= tempreturnList->frequency/2;
    tempreturnList = tempreturnList->next;
  }
  // at this point returnList should be at the last element
  return returnList;
}

double KLD(wordList* f1, wordList* f2){
  // f2 would be the mean list
  double result = 0.0;
  wordList* temp = f1;
  if(strcmp(temp->word, "\0") == 0)return 0;
  while(temp != NULL){
    // if its in the f2, then calculate the two frequencies
    if(lookUpwordList(f2, temp->word) == true){
      double f2FrequencyValue = getFrequency(f2, temp->word);
      //printf("get freq: %f\n", f2FrequencyValue);
      double log1 = log(temp->frequency/f2FrequencyValue);
      result += temp->frequency * (log1/log(2));
    //  printf("this is result");
    }
    temp = temp->next;
  }
  return result;
}

double JSD(wfdRepo* r1, wfdRepo* r2){
  wordList* meanfList = getMeanFreqlist(r1->wfd, r2->wfd);
  //printwordListfreq(meanfList);
  double kldval1 = KLD(r1->wfd, meanfList);
  double kldval2 = KLD(r2->wfd, meanfList);
  destroywordList(meanfList);

  return sqrt((kldval1+kldval2)/2);
}


//repoQueue stuff

//inits repoQueue with all pairs of files
void init_repoQueue(repoQueue* l, wfdRepo* repo){
  //assumes l is malloced
  l->repo1 = NULL;
  l->next = NULL;
  repoQueue* templist = l;
  wfdRepo* temp1 = repo;
  while(temp1->next!=NULL){
    wfdRepo* temp2 = temp1->next;
    while(temp2!=NULL)
    {
        templist->repo1 = temp1;
        templist->repo2 = temp2;

        templist->next = malloc(sizeof(repoQueue));
        templist->next->repo1 = NULL;
        templist = templist->next;

      temp2 = temp2->next;

    }
    temp1 = temp1->next;
  }
  if(templist == l){
    return;
  }
  if(templist->repo1 == NULL){

    repoQueue* t = l;
    while( t->next->repo1!=NULL)
    {
      t= t->next;
    }
    free(templist);
    t->next = NULL;
  }
  return;
}

void print_repoQueue(repoQueue* l){
  printf("--------------------PRINTING REPOQUEUE--------------------\n");
  int count = 0;
  repoQueue* temp = l;
  if(temp->repo1 == NULL) printf(" empty\n");
  else while(temp !=NULL)
  {
    printf("%d| ", count);
    printf("file 1: %s | ", temp->repo1->fileName);
    printf("file 2: %s ", temp->repo2->fileName);
    printf("\n");
    count++;
    temp = temp->next;
  }
  printf("---------------------------------------------------------\n\n");
}

void destroy_repoQueue(repoQueue* l){
  if(l ==NULL) return;
  if(l->next !=NULL) destroy_repoQueue(l->next);
  free(l);
}

repoQueue* dequeue_repoQueue(repoQueue* q){
  repoQueue* temp = q;
  if(temp == NULL) return q;
  if(temp->repo1 == NULL){
    free(q);
    return NULL;
  }
  q = q->next;
  free(temp);
  return q;

}

//JSDList Stuff

void enqueue_JSDList(JSDList* q, char* name1, char* name2, double JSDval, int occ){
  if(q->totalOcc == -1)
  {
    memcpy(q->fileName1, name1, strlen(name1)+1);
    memcpy(q->fileName2, name2, strlen(name2)+1);
    q->JSDval = JSDval;
    q->totalOcc = occ;
    q->next = NULL;
    return;
  }
  while(q->next != NULL){
    q = q->next;
  }
  JSDList* newNode = malloc(sizeof(JSDList));
  memcpy(newNode->fileName1, name1, strlen(name1)+1);
  memcpy(newNode->fileName2, name2, strlen(name2)+1);
  newNode->JSDval = JSDval;
  newNode->totalOcc = occ;
  newNode->next = NULL;

  q->next = newNode;
}

void destroy_JSDList(JSDList* l){
  if(l ==NULL) return;
  if(l->next !=NULL) destroy_JSDList(l->next);
  free(l);
}

void* threadJSD(void* targs){
  pthread_mutex_lock(&anLock);
  repoQueue** rq = ((anThreadArgs*)targs)->rq;
  JSDList* jl = ((anThreadArgs*)targs)->jl;
  while(*rq!=NULL)
  {
    wfdRepo* repo1 = (*rq)->repo1;
    wfdRepo* repo2 = (*rq)->repo2;
    (*rq) = dequeue_repoQueue(*rq);
    if(repo1!=NULL)
    {
    pthread_mutex_unlock(&anLock);
    double JSDval = JSD(repo1, repo2);
    int total = repo1->total+repo2->total;
    pthread_mutex_lock(&anLock);
    enqueue_JSDList(jl, repo1->fileName, repo2->fileName, JSDval, total);
    }
  }
  pthread_mutex_unlock(&anLock);
  return NULL;
}
JSDList* findMax_JSDList(JSDList *l){
  //finds item with highest occurrences and sets the totalOcc to 0
  JSDList* temp = l;
  JSDList* max = temp;
  while(temp !=NULL){
    if(temp->totalOcc > max->totalOcc) max = temp;
    temp = temp->next;
  }
  return max;
}

void print_JSDList(JSDList *l){
  JSDList* temp = findMax_JSDList(l);
  while(temp ->totalOcc !=-1){
    //  printf("(%d)", temp->totalOcc);
    temp->totalOcc = -1;
    printf("%f %s %s ", temp->JSDval, temp->fileName1, temp->fileName2);
    printf("\n");
    temp = findMax_JSDList(l);
  }
}
//--------------------------------MISC STUFF-------------------------------

bool isWhite(char c){
  switch(c){
    case(' '):
    case('\n'):
    case('\t'):
    case('\r'):
    case('\v'):
    return true;
    default: break;
  }
  return false;
}
// goes through the file and add the words to wfdlist
void wordCount(int fd, wordList* wfdlist){
    // lock here
    char bufchar='?';
    //initialize word struct pointer
    strbuf_t* word = malloc(sizeof(strbuf_t));
    sb_init(word, 1);
    //read one char at a time
    int bytes_read = read(fd, &bufchar, 1);
    //while there is still bytes to read
    while (bytes_read > 0) {
      bufchar = tolower(bufchar);
      while(!isWhite(bufchar)){
        if(bufchar == '-' || !ispunct(bufchar)){
          sb_append(word, bufchar);
        }
        bytes_read = read(fd, &bufchar, 1);
      }
      //from here on out, its a white space
      char* wordstr= calloc(word->used+1, sizeof(char));
      sb_toString(word, wordstr);
      // if it doesnt wwork, add null terminator at the end
      if(strlen(wordstr)>0){
          insertwordList(wfdlist, wordstr);
      }
      else free(wordstr);
      sb_destroy(word);
      sb_init(word, 1);
      bytes_read = read(fd, &bufchar, 1);
    }
    sb_destroy(word);
    free(word);
    close(fd);
    // unlock
}

void* threadFile(void* tArgs){
  pthread_mutex_lock(&colLock);
  Queue** files = ((threadArgs*)tArgs)->files;
  wfdRepo* repo = ((threadArgs*)tArgs)->repo;
  int* filethreadAmt = &((threadArgs*)tArgs)->filethreadAmt;
  wfdRepo* temp = repo;

  //if files is empty
  if(*files!=NULL && (*files)->name[0] == '\0')
  {
    free(*files);
    *files = NULL;
  }
  while(*files != NULL ){
    memcpy(temp->fileName, (*files)->name, strlen((*files)->name)+1);

    *files = dequeue(*files);

    int fd = open(temp->fileName, O_RDONLY);
    if(fd == -1){
      printf("the file is %s\n", temp->fileName);
      perror("can not open file");
    }
    else{
      wordCount(fd, temp->wfd);
      if(*files != NULL && (*files) -> name[0] !='\0'){
        wfdRepo* newNode = malloc(sizeof(wfdRepo));
        init_wfdRepo(newNode);
        temp->next = newNode;
        temp = temp->next;
      }
      else if(*files!=NULL){
        //this should  never happen, but just in case.
        free(*files);
        *files = NULL;
      }
      if(*files==NULL){
          (*filethreadAmt)--;
          if(*filethreadAmt == 0){
            // broadcast to continue every dthread
            pthread_cond_broadcast(&fileSig);
            pthread_mutex_unlock(&colLock);
            return NULL;
          }
          //  wait
          pthread_cond_wait(&fileSig, &colLock);

          if((*files)==NULL)
          {
            pthread_cond_broadcast(&fileSig);
            pthread_mutex_unlock(&colLock);
            return NULL;
          }
          (*filethreadAmt)++;
        }
    }

  }
  pthread_cond_broadcast(&fileSig);
  pthread_mutex_unlock(&colLock);
  return 0;
}

void* threadDirectory(void* tArgs){
  pthread_mutex_lock(&colLock);
  int* dirthreadAmt = &(((threadArgs*)tArgs)->dirthreadAmt);
  Queue** files = ((threadArgs*)tArgs)->files;
  Queue** directories = ((threadArgs*)tArgs)->directories;
  char* suffix = ((threadArgs*)tArgs)->suffix;
  while((*directories) != NULL){

    char dirName[STRING_MAX] = "";
    memcpy(dirName, (*directories)->name, strlen((*directories)->name)+1);

    //pthread_mutex_lock(&colLock);
    *directories = dequeue(*directories);
    /*we init directories if NULL so that if checkDirectory tries to enqueue,
    there will be no issue*/
    if(*directories == NULL)
      {
        *directories = malloc(sizeof(Queue));
        init_queue(*directories);
      }

    checkDirectory(dirName, *files, *directories, suffix);
    if((*directories)->name[0] == '\0'){
       free(*directories);
       *directories = NULL;
     }

    if(*directories==NULL){
      (*dirthreadAmt)--;
      if(*dirthreadAmt == 0){
        // broadcast to continue every dthread
        pthread_cond_broadcast(&dirSig);
        pthread_mutex_unlock(&colLock);
        return NULL;
      }
      //  wait
      pthread_cond_wait(&dirSig, &colLock);

      if((*directories)==NULL)
      {
          pthread_mutex_unlock(&colLock);
        pthread_cond_broadcast(&dirSig);
        return NULL;
      }
      (*dirthreadAmt)++;
    }
  }
  pthread_mutex_unlock(&colLock);
  pthread_cond_broadcast(&dirSig);
  return NULL;
}

bool checkValidFileSuffix(char* fileName, char* suffix){
  int suffixLength = strlen(suffix);
  int fileLength = strlen(fileName);
  if(suffixLength > fileLength) return false;
  int counter = 0;
  for(int i = fileLength-suffixLength; i<fileLength; i++){
    if(fileName[i] != suffix[counter]){
      return false;
    }
    counter++;
  }
  return true;
}

void checkDirectory(char* dirname, Queue* files, Queue* directories, char* suffix){
    DIR* dir = opendir(dirname);
    if(dir == NULL){
      return;
    }
    struct dirent* entity;
    entity = readdir(dir);
    while(entity != NULL){
      //dequeue the current directory from directories
      if(strcmp(entity->d_name, ".") != 0 && strcmp(entity->d_name, "..") != 0){
        // might need to redo this part so that it would check if theres anymore files in directories
        if(entity->d_type == DT_DIR){
          char path[STRING_MAX] = "";
          strcat(path, dirname);
          strcat(path, "/");
          strcat(path, entity->d_name);
          path[strlen(path)] = '\0';

          //enqueue if a directory is found
          enqueue(directories, path);
          pthread_cond_signal(&dirSig);
            // signal  one other thread to continue
          // signal that a directory is made
        }
        else if(entity->d_type == DT_REG){
          char path[STRING_MAX] = "";
          strcat(path, dirname);
          strcat(path, "/");
          strcat(path, entity->d_name);
          path[strlen(path)] = '\0';
          if(checkValidFileSuffix(path, suffix) == true){
            // do word count here
            enqueue(files, path);
            // signal  one file thread to continue
            pthread_cond_signal(&fileSig);
          }
        }
      }
      entity = readdir(dir);
    }
    closedir(dir);
}

int isdir(char *name) {
    struct stat data;
    int err = stat(name, &data);
    if (err) {
        return false;
    }
    if (S_ISDIR(data.st_mode)) {
        // S_ISDIR macro is true if the st_mode says the file is a directory
        // S_ISREG macro is true if the st_mode says the file is a regular file
        return true;
    }
    return false;
}

bool file_exists (char *filename) {
  FILE *file;
  if((file = fopen(filename, "r"))){
    fclose(file);
    return true;
  }
  return false;
}

int main(int argv, char* argc[argv+1]){
//------------------------INIT PHASE--------------------------
  //TODO make a destroy for wfdRepo
  if(argv == 1){
    perror("less than 2 files");
    return EXIT_FAILURE;
  }
  char* suffix = ".txt";
  int DTNumber = 1;
  int FTNumber = 1;
  int ATNumber = 1;
  Queue* directories = malloc(sizeof(Queue));
  init_queue(directories);

  Queue* files = malloc(sizeof(Queue));
  init_queue(files);
  bool suffixchanged = false;
  for(int i = 1; i < argv; i++){
    //printf("%d\n", i);
    if(argc[i][0] == '-'){
      // change optional args here
      //printf("change optional argument\n");
      char compare = argc[i][1];
      int length = strlen(argc[i]);
      if((length < 3 && compare != 's') || (compare != 'd' && compare != 'a' && compare != 'f' && compare != 's')){
        printf("error setting flags\n");
        destroyQueue(files);
        destroyQueue(directories);
        return EXIT_FAILURE;
      }
      char* restOfString = malloc(sizeof(char)*(length-1));
      for(int j = 2; j < length; j++){
        restOfString[j-2] = argc[i][j];
      }
      restOfString[length-2] = '\0';
      if(compare == 'd'){
        //change in here
        DTNumber = atoi(restOfString);
        //printf("this is DTNumber %d\n", DTNumber);
      }
      else if(compare == 'f'){
        //change in here
        FTNumber = atoi(restOfString);;
      }
      else if(compare == 'a'){
        //change in here
        ATNumber = atoi(restOfString);;
      }
      if(compare == 's'){
        //change in here
        suffix = restOfString;
        suffixchanged = true;
        //printf("this is suffix after changing: %s\n", suffix);
      }

      else free(restOfString);
    }
    else if(isdir(argc[i])){
      enqueue(directories, argc[i]);
    }
    // check if input is valid
    else if(file_exists(argc[i])){
      enqueue(files, argc[i]);
    }
    else{
      perror("file or directory not found");
    }
  }

  if(DTNumber <= 0 || FTNumber <= 0 || ATNumber <= 0){
    perror("error setting flags\n");
    destroyQueue(files);
    destroyQueue(directories);
    return EXIT_FAILURE;
  }
  pthread_mutex_init(&colLock, NULL);
  pthread_cond_init(&dirSig, NULL);
  pthread_cond_init(&fileSig, NULL);

  wfdRepo* repo = malloc(sizeof(wfdRepo));
  init_wfdRepo(repo);

  //------------------------COLLECTION PHASE--------------------------

  threadArgs* t_args = malloc( sizeof(threadArgs));
  t_args-> files = &files;
  t_args-> directories = &directories;
  t_args-> suffix = suffix;
  t_args-> repo = repo;
  t_args-> dirthreadAmt = DTNumber;
  t_args-> filethreadAmt = FTNumber;

  pthread_t* fileThread = malloc(sizeof(pthread_t)*FTNumber);
  pthread_t* dirThread = malloc(sizeof(pthread_t)*DTNumber);

  for(int i = 0; i < DTNumber; i++){
    int success = pthread_create(&dirThread[i], NULL, threadDirectory, t_args);
    if(success != 0){
      perror("failed to create a thread");
      destroyQueue(files);
      destroyQueue(directories);
      destroywfdRepo(repo);
      pthread_cond_destroy(&dirSig);
      pthread_cond_destroy(&fileSig);
      pthread_mutex_destroy(&colLock);
      free(t_args);
      free(fileThread);
      free(dirThread);
      return EXIT_FAILURE;
    }
    //pthread create dir
  }
  for(int i = 0; i < FTNumber; i++){
    //pthread create file
    int success = pthread_create(&fileThread[i], NULL, threadFile, t_args);
    if(success != 0){
      perror("failed to create a thread");
      destroyQueue(files);
      destroyQueue(directories);
      destroywfdRepo(repo);
      pthread_cond_destroy(&dirSig);
      pthread_cond_destroy(&fileSig);
      pthread_mutex_destroy(&colLock);
      free(t_args);
      free(fileThread);
      free(dirThread);
      return EXIT_FAILURE;
    }
  }

  for(int i = 0; i < DTNumber; i++){
    //pthread join directory
      int success = pthread_join(dirThread[i], NULL);
      if(success != 0){
        perror("failed to join a thread");
        destroyQueue(files);
        destroyQueue(directories);
        destroywfdRepo(repo);
        pthread_cond_destroy(&dirSig);
        pthread_cond_destroy(&fileSig);
        pthread_mutex_destroy(&colLock);
        free(t_args);
        free(fileThread);
        free(dirThread);
        return EXIT_FAILURE;
      }
  }
  for(int i = 0; i < FTNumber; i++){
    //pthread join file
      int success = pthread_join(fileThread[i], NULL);
      if(success != 0){
        perror("failed to join a thread");
        destroyQueue(files);
        destroyQueue(directories);
        destroywfdRepo(repo);
        pthread_cond_destroy(&dirSig);
        pthread_cond_destroy(&fileSig);
        pthread_mutex_destroy(&colLock);
        free(t_args);
        free(fileThread);
        free(dirThread);
        return EXIT_FAILURE;
      }
  }
  setTotal_wfdRepo(repo);
  setFrequencies(repo);

  //printwfdRepo(repo);
//  printf("here?\n");
  if(repo == NULL|| repo->fileName[0] == '\0' || repo->next == NULL){
    perror("less than 2 files");
    free(files);
    free(directories);
    destroywfdRepo(repo);
    pthread_cond_destroy(&dirSig);
    pthread_cond_destroy(&fileSig);
    pthread_mutex_destroy(&colLock);
    free(t_args);
    free(fileThread);
    free(dirThread);
    return EXIT_FAILURE;
  }

//------------------------ANALYSIS PHASE--------------------------
  pthread_mutex_init(&anLock, NULL);
  repoQueue* rqueue = malloc(sizeof(repoQueue));
  init_repoQueue(rqueue, repo);
  //print_repoQueue(rqueue);
  JSDList* jlist = malloc(sizeof(JSDList));
  jlist->totalOcc = -1;
  jlist->next = NULL;

  anThreadArgs* at_args = malloc(sizeof(anThreadArgs));
  at_args->rq = &rqueue;
  at_args -> jl = jlist;

  //By now, we should have all the frequencies and stuff set
  pthread_t* analysisThread = malloc(sizeof(pthread_t)*ATNumber);
  for(int i = 0; i < ATNumber; i++){
    // maybe create a struct that would store the result of analysis?
      int success = pthread_create(&analysisThread[i], NULL, threadJSD, at_args);
      if(success != 0){
        perror("failed to create a thread");
        destroy_repoQueue(rqueue);
        destroy_JSDList(jlist);
        free(at_args);
        destroyQueue(files);
        destroyQueue(directories);
        destroywfdRepo(repo);
        pthread_cond_destroy(&dirSig);
        pthread_cond_destroy(&fileSig);
        pthread_mutex_destroy(&colLock);
        free(t_args);
        free(fileThread);
        free(dirThread);
        free(analysisThread);
        return EXIT_FAILURE;
      }
  }
  for(int i = 0; i < ATNumber; i++){
      int success = pthread_join(analysisThread[i],NULL);
      if(success != 0){
        perror("failed to join a thread");
        destroy_repoQueue(rqueue);
        destroy_JSDList(jlist);
        free(at_args);
        destroyQueue(files);
        destroyQueue(directories);
        destroywfdRepo(repo);
        pthread_cond_destroy(&dirSig);
        pthread_cond_destroy(&fileSig);
        pthread_mutex_destroy(&colLock);
        free(t_args);
        free(fileThread);
        free(dirThread);
        free(analysisThread);
        return EXIT_FAILURE;
      }
  }

    //printf("________________________PRINTING JSDLIST________________________\n");
  print_JSDList(jlist);
    //printf("________________________________________________________________\n\n");

  //free all the malloced stuff
  if(suffixchanged == true) free(suffix);
  free(at_args);
  destroy_JSDList(jlist);
  destroy_repoQueue(rqueue);
  pthread_cond_destroy(&fileSig);
  pthread_cond_destroy(&dirSig);
  pthread_mutex_destroy(&colLock);
  pthread_mutex_destroy(&anLock);
  destroywfdRepo(repo);
  free(t_args);
  free(analysisThread);
  free(files);
  free(directories);
  free(fileThread);
  free(dirThread);
  return EXIT_SUCCESS;
}
