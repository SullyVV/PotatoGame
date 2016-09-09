#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/select.h>
#include <fcntl.h>

#include "potato.h"

int main(int argc, char *argv[])
{
  if (argc != 2) {
    printf("wrong number of inputs arguments\n");
    exit(0);
  }
  srand((unsigned int)time(NULL));
  char* player_ID = argv[1];
  char* dir = "/tmp/";
  
  //open the fifos connected to ringmaster
  char p_m[32];
  char m_p[32];
  strcpy(m_p, dir);
  strcat(m_p, "master_p");
  strcat(m_p, player_ID);
  strcpy(p_m, dir);
  strcat(p_m, "p");
  strcat(p_m, player_ID);
  strcat(p_m, "_master");
  int rd_fd[3]; //[0]: from master. [1]: from left. [2]: from right
  int wr_fd[3]; //[0]: to master. [1]: to left. [2]: to right
  rd_fd[0] = open(m_p, O_RDONLY);
  wr_fd[0] = open(p_m, O_WRONLY);
  //get the message from the master
  char mesg[64];
  while(1) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(rd_fd[0], &rfds);
    if(select(rd_fd[0]+1, &rfds, NULL, NULL, NULL) == 1) {
      read(rd_fd[0], &mesg, 64);
      printf("%s\n", mesg);
      break;
    }
  }
  //get the total number of players
  int total;
  while(1) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(rd_fd[0], &rfds);
    if (select(rd_fd[0]+1, &rfds, NULL, NULL, NULL) == 1) {
      read(rd_fd[0], &total, sizeof(total));
      break;
    }
  }
  //get pathname of fifos connected to neighbours
  char pl_pc[32];
  char pc_pl[32];
  char pr_pc[32];
  char pc_pr[32];
  char left[10];
  char right[10];
  int curr = atoi(player_ID);
  int l;
  int r;
  if (curr == 0) {
    l = curr + 1;
    r = total - 1;
  } else if (curr == total - 1) {
    l = 0;
    r = curr - 1;
  } else {
    l = curr + 1;
    r = curr - 1;
  }
  sprintf(left, "%d", l);
  sprintf(right, "%d", r);
  strcpy(pl_pc, dir);
  strcat(pl_pc, "p");
  strcat(pl_pc, left);
  strcat(pl_pc, "_p");
  strcat(pl_pc, player_ID);
  strcpy(pc_pl, dir);
  strcat(pc_pl, "p");
  strcat(pc_pl, player_ID);
  strcat(pc_pl, "_p");
  strcat(pc_pl, left);
  strcpy(pr_pc, dir);
  strcat(pr_pc, "p");
  strcat(pr_pc, right);
  strcat(pr_pc, "_p");
  strcat(pr_pc, player_ID);
  strcpy(pc_pr, dir);
  strcat(pc_pr, "p");
  strcat(pc_pr, player_ID);
  strcat(pc_pr, "_p");
  strcat(pc_pr, right);
  //open fifos connected to neighbours
  if (curr == 0) {
    rd_fd[1] = open(pl_pc, O_RDONLY);
    wr_fd[1] = open(pc_pl, O_WRONLY);
    wr_fd[2] = open(pc_pr, O_WRONLY);
    rd_fd[2] = open(pr_pc, O_RDONLY);
  }else{ 
    wr_fd[2] = open(pc_pr, O_WRONLY);
    rd_fd[2] = open(pr_pc, O_RDONLY);
    rd_fd[1] = open(pl_pc, O_RDONLY);
    wr_fd[1] = open(pc_pl, O_WRONLY);
  }
  //receive the potato from the master or left or right neighbour
  //if received a mesg says finishes, exit
  int r_max;
  r_max = rd_fd[0] > rd_fd[1] ? rd_fd[0] : rd_fd[1];
  r_max = r_max > rd_fd[2] ? r_max : rd_fd[2];
  while(1){					
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(rd_fd[0], &rfds);
    FD_SET(rd_fd[1], &rfds);
    FD_SET(rd_fd[2], &rfds);
    int a;
    if (a = select(r_max+1, &rfds, NULL, NULL, NULL) > 0) {
      //printf("a is %d\n", a);
      if (FD_ISSET(rd_fd[0], &rfds)) {
	//printf("read from master\n");
	POTATO_T p;
	int cnt = read(rd_fd[0], &p, sizeof(POTATO_T));
	//printf("cnt is %d\n", cnt);
	if (cnt > 1000) {
	  //for potato from master
	  p.hop_trace[p.total_hops - p.hop_count] = curr;
	  p.hop_count = p.hop_count - 1;
	  if (p.hop_count == 0) {
	    //send back to master
	    printf("I'm it\n");
	    write(wr_fd[0], &p, sizeof(POTATO_T));
	  } else {
	    //send to left or right
	    int random = rand()%10;
	    if (random%2 == 0) {
	      //send to left neighbour
	      printf("Sending potato to %d\n", l);
	      write(wr_fd[1], &p, sizeof(POTATO_T));
	    }else{
	      //send to right neighour
	      printf("Sending potato to %d\n", r);
	      write(wr_fd[2], &p, sizeof(POTATO_T));
	    }
	  }
	} else if (cnt == 0){
	  //do nothing
	} else {
	  //receive exit signal from master, close all fd
	  for (int i = 0; i < 3; i++) {
	    if (close(rd_fd[i]) != 0) {
	      perror("failed to close fd");
	    }
	    if (close(wr_fd[i]) != 0) {
	      perror("failed to close fd");
	    }
	  }
	  break;
	}
      }
      if (FD_ISSET(rd_fd[1], &rfds)) {
	//get potato from left neighbour
	POTATO_T p;
	int cnt = read(rd_fd[1], &p, sizeof(POTATO_T));
	//printf("cnt is %d\n", cnt);
	if (cnt > 1000) {
	  p.hop_trace[p.total_hops - p.hop_count] = curr;
	  p.hop_count = p.hop_count - 1;
	  if (p.hop_count == 0) {
	    //send back to master
	    printf("I'm it\n");
	    write(wr_fd[0], &p, sizeof(POTATO_T));
	  } else {
	    //send to left or right
	    int random = rand()%10;
	    if (random%2 == 0) {
	      //send to left neighbour
	      printf("Sending potato to %d\n", l);
	      write(wr_fd[1], &p, sizeof(POTATO_T));
	    }else{
	      //send to right neighour
	      printf("Sending potato to %d\n", r);
	      write(wr_fd[2], &p, sizeof(POTATO_T));
	    }
	  }
	}
      }
      if (FD_ISSET(rd_fd[2], &rfds)) {
	//printf("read from right\n");
	//break;
	POTATO_T p;
	int cnt = read(rd_fd[2], &p, sizeof(POTATO_T));
	//printf("cnt is %d\n", cnt);
	if (cnt > 1000) {
	  //get potato from right neighbour
	  p.hop_trace[p.total_hops - p.hop_count] = curr;
	  p.hop_count = p.hop_count - 1;
	  if (p.hop_count == 0) {
	    //send back to master
	    printf("I'm it\n");
	    write(wr_fd[0], &p, sizeof(POTATO_T));
	  } else {
	    //send to left or right
	    int random = rand()%10;
	    if (random%2 == 0) {
	      //send to left neighbour
	      printf("Sending potato to %d\n", l);
	      write(wr_fd[1], &p, sizeof(POTATO_T));
	    }else{
	      //send to right neighour
	      printf("Sending potato to %d\n", r);
	      write(wr_fd[2], &p, sizeof(POTATO_T));
	    }
	  }
	}
      }
    }
  }
  exit(0);
}
