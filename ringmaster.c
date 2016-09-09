#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/select.h>
#include <fcntl.h>

#include "potato.h"


int main(int argc, char *argv[])
{
  //get number of players and hops
  if (argc != 3) {
    printf("wrong number of input arguments.\n");
    return 0;
  }
  int num_players = atoi(argv[1]);
  int num_hops = atoi(argv[2]);
  printf("Potato Ringmaster\n");
  printf("Players = %d\n", num_players);
  printf("Hops = %d\n", num_hops);
  //set names of all fifos needed
  char** master_player = malloc(num_players * sizeof(*master_player));
  char** player_master = malloc(num_players * sizeof(*player_master));
  char** pf_pl = malloc(num_players * sizeof(*pf_pl));
  char** pl_pf = malloc(num_players * sizeof(*pl_pf));
  char* dir = "/tmp/";
  //for fifos between master and player
  for(int i = 0; i < num_players; i++) {
    master_player[i] = malloc(32 * sizeof(char));
    player_master[i] = malloc(32 * sizeof(char));
    char num[10];
    sprintf(num, "%d", i);
    strcpy(master_player[i], dir);
    strcat(master_player[i], "master_p");
    strcat(master_player[i], num);
    strcpy(player_master[i], dir);
    strcat(player_master[i], "p");
    strcat(player_master[i], num);
    strcat(player_master[i], "_master");
  }
  //for fifos between player and player
  for(int i = 0; i < num_players-1; i++) {
    pf_pl[i] = malloc(32 * sizeof(char));
    pl_pf[i] = malloc(32 * sizeof(char));
    char num1[10];
    char num2[10];
    sprintf(num1, "%d", i);
    sprintf(num2, "%d", i+1);
    strcpy(pf_pl[i], dir);
    strcat(pf_pl[i], "p");
    strcat(pf_pl[i], num1);
    strcat(pf_pl[i], "_p");
    strcat(pf_pl[i], num2);
    strcpy(pl_pf[i], dir);
    strcat(pl_pf[i], "p");
    strcat(pl_pf[i], num2);
    strcat(pl_pf[i], "_p");
    strcat(pl_pf[i], num1);
  }
  pf_pl[num_players-1] = malloc(32 * sizeof(char));
  pl_pf[num_players-1] = malloc(32 * sizeof(char));
  char num1[10];
  char num2[10];
  sprintf(num1, "%d", num_players-1);
  sprintf(num2, "%d", 0);
  strcpy(pf_pl[num_players-1], dir);
  strcat(pf_pl[num_players-1], "p");
  strcat(pf_pl[num_players-1], num1);
  strcat(pf_pl[num_players-1], "_p");
  strcat(pf_pl[num_players-1], num2);
  strcpy(pl_pf[num_players-1], dir);
  strcat(pl_pf[num_players-1], "p");
  strcat(pl_pf[num_players-1], num2);
  strcat(pl_pf[num_players-1], "_p");
  strcat(pl_pf[num_players-1], num1);
  //unlink all fifos first
  for(int i = 0; i < num_players; i++) {
    if(unlink(master_player[i]) != 0) {
      perror("failed to unlink");
    }
    if (unlink(player_master[i]) != 0) {
      perror("failed to unlink");
    }
    if (unlink(pf_pl[i]) != 0) {
      perror("failed to unlink");
    }
    if (unlink(pl_pf[i]) != 0) {
      perror("failed to unlink");
    }
  }
  //create fifos
  for(int i = 0; i < num_players; i++) {
    if (mkfifo(master_player[i], S_IRUSR|S_IWUSR)) {
      perror("failed to makefifo");
    }
    if (mkfifo(player_master[i], S_IRUSR|S_IWUSR)) {
      perror("failed to makefifo");
    }
    if (mkfifo(pf_pl[i], S_IRUSR|S_IWUSR)) {
      perror("failed to makefifo");
    }
    if (mkfifo(pl_pf[i], S_IRUSR|S_IWUSR)) {
      perror("failed to makefifo");
    }
  }
  int m_p[num_players]; //fd for write to players
  int p_m[num_players]; //fd for read from players
  int r_max = -1;
  int w_max = -1;
  //open fifos from master to players and from players to master and send a mesg to each player
  for (int i = 0; i < num_players; i++) {
    m_p[i] = open(master_player[i], O_WRONLY);
    if (m_p[i]) {
      printf("Player %d is ready to play\n", i);
    }
    if (m_p[i] > w_max) {
      w_max = m_p[i];
    }
    char mesg[64];
    char num[10];
    char total[10];
    sprintf(num, "%d", i);
    sprintf(total, "%d", num_players);
    strcpy(mesg, "Connected as player ");
    strcat(mesg, num);
    strcat(mesg, " out of ");
    strcat(mesg, total);
    strcat(mesg, " total players");
    write(m_p[i], &mesg, 64);
    write(m_p[i], &num_players, sizeof(num_players));
    p_m[i] = open(player_master[i], O_RDONLY);
    if (p_m[i] > r_max) {
      r_max = p_m[i];
    }
  }
  //All players are ready, choose a player to start randomly
  srand((unsigned int)time(NULL));
  int random = rand()%num_players;
  printf("All players present, sending potato to player %d\n", random);
  //create a potato and send it to a random player
  
  POTATO_T p;
  p.total_hops = num_hops;
  p.hop_count = num_hops;
  write(m_p[random], &p, sizeof(POTATO_T));
  //wait for one player send back the potato
  while(1) {
    fd_set rfds;
    FD_ZERO(&rfds);
    for (int i = 0; i < num_players; i++) {
      FD_SET(p_m[i], &rfds);
    }
    if (select(w_max+1, &rfds, NULL, NULL, NULL) > 0) {
      for (int i = 0; i < num_players; i++) {
          if (FD_ISSET(p_m[i], &rfds)) {
              //printf("potato from %d\n", i);
              int cnt = read(p_m[i], &p, sizeof(POTATO_T));
              //printf("read %d bytes\n", cnt);
              break;
          }
      }
      break;
    }
  }
  //show trace
  printf("Trace of potato:\n");
  for (int i = 0; i < num_hops-1; i++) {
    printf("%d,", p.hop_trace[i]);
  }
  printf("%d\n", p.hop_trace[num_hops-1]);
  //send exit mesg to all players
  char quit[32];
  strcpy(quit, "exit");
  for (int i = 0; i < num_players; i++) {
    write(m_p[i], &quit, 32);
  }
  //close all fds and unlink fifos
  for (int i = 0; i < num_players; i++) {
    if (close(p_m[i]) != 0) {
      perror("failed to close fd");
    }
    if (close(m_p[i]) != 0) {
      perror("failed to close fd");
    }
  }
  exit(0);
}

