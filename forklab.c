/* A virtual controller of several robot units. 
Run it to get usage instructions.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>

#define N 10
#define M 10


int active_num = 0; //number of active units
int *alive;	//array of status flags
int **fd;	//array of file descriptors for all pipes
int term_id[2]; //a file descripter for transmitting a killed unit id

void clear_buf(char *buf) {
	int i;
	for(i = 0; i < 32; i++)	//clearing buffer
		buf[i] = '\0';
}

int get_steps(char *buf) {
	char steps[32];
	char *space;
	space = strchr(buf, ' ');
	clear_buf(steps);
	strcpy(steps, space);
	return atoi(steps);
}

void new_unit(int file_desc, int id, int pos_pipe) {	//Creates new child process and starts its action loop
	char buf[32];
	int pos_x, pos_y, d_x, d_y;
	int i, steps;

	if(fork() == 0) {
		pos_x = 0;
		pos_y = 0;
		d_x = 0;
		d_y = 0;

		clear_buf(buf);
		snprintf(buf, 32, "%d %d %d %d %d", id, pos_x - d_x, pos_y - d_y, pos_x, pos_y);
		write(pos_pipe, buf, 32);	//sending coordinates to visualizator
		while(1) {
		    read(file_desc, buf, 32);
			if(strncmp(buf, "k", 1) == 0) {
				close(file_desc);
				clear_buf(buf);
				close(1);
				dup(term_id[1]);
				snprintf(buf, 32, "%d", id);
				write(term_id[1], buf, 4);
				exit(0);
			}
			d_x = 0;
			d_y = 0;
			if(strncmp(buf, "p", 1) == 0) {
				fprintf(stdout, "INFO: Unit %d position: [%d;%d]\n", id + 1, pos_x, pos_y);
				continue;
			}
			if(strncmp(buf,"u", 1) == 0) {
				fprintf(stdout, "INFO: Unit %d goes up.\n", id + 1);
				d_x = 0;
				d_y = 1;
			}
			if(strncmp(buf, "d", 1) == 0) {
				fprintf(stdout, "INFO: Unit %d goes down.\n", id + 1);
				d_x = 0;
				d_y = -1;
			}
			if(strncmp(buf, "l", 1) == 0) {
				fprintf(stdout, "INFO: Unit %d goes left.\n", id + 1);
				d_x = -1;
				d_y = 0;
			}
			if(strncmp(buf, "r", 1) == 0) {
				fprintf(stdout, "INFO: Unit %d goes right.\n", id + 1);
				d_x = 1;
				d_y = 0;
			}
			
			if(d_x == 0 && d_y == 0) {
				fprintf(stderr, "ERROR: Unrecognized command \"%s\".\n", buf);
				continue;	
			}
			else {
				steps = get_steps(buf);
				for(i = 0; i < steps; i++) {
					if ((pos_x + d_x < 0 || pos_x + d_x > N) || (pos_y + d_y < 0 || pos_y + d_y > M)) {	//go until the wall
						fprintf(stderr, "WARNING: Unit %d reached the wall at [%d;%d]\n", id + 1, pos_x, pos_y);
						break;
					}
					sleep(1);
					pos_x += d_x;
					pos_y += d_y;

					clear_buf(buf);
					snprintf(buf, 32, "%d %d %d %d %d", id, pos_x - d_x, pos_y - d_y, pos_x, pos_y);
					write(pos_pipe, buf, 32);	//sending coordinates to visualizator
				}
				fprintf(stdout, "INFO: Unit %d stopped at [%d;%d]\n", id + 1, pos_x, pos_y);
			}
		}
	}
}

void clear_screen() {
	struct termios savetty;
  	struct termios tty;

	////clearing a screen/////
	if ( !isatty(0) ) {
		fprintf (stderr, "ERROR: Unable to set mode.\n");
		exit (1);
	};
	tcgetattr (0, &tty);
	savetty = tty;
	tty.c_lflag &= ~(ICANON|ECHO|ISIG);
	tty.c_cc[VMIN] = 1;
	tcsetattr (0, TCSAFLUSH, &tty);	
	printf("%c[2J", 27);
  	fflush(stdout);
  	printf("%c[%d;%dH", 27, 0, 0);	//setting cursor to [0;0]
  	fflush(stdout);
  	tcsetattr (0, TCSAFLUSH, &savetty);
  	///////
}

void clrscr() {	//another version of screen clearer (if the first one won't work)
    unsigned char esc[11];
    esc[0] = 27;
    esc[1] = '[';
    esc[2] = 'H';
    write(1, esc, 3);
    esc[2] = '2';
    esc[3] = 'J';
    write(1, esc, 4);
    return;
}

void visualizer(int pos_pipe) {
  	char id;
  	int i, j, x, y, prev_x, prev_y;
  	char **coord;
  	char buf[32];

	if(fork() == 0) {
		id = 0;
		x = 0;
    	y = 0;
    	prev_x = 0;
    	prev_y = 0;
    	coord = (char**)malloc(N * sizeof(char*));
    	for(i = 0; i < N; ++i) {
    		coord[i] = (char*)malloc(M * sizeof(char));
    		for(j = 0; j < M; ++j)
    			coord[i][j] = ' ';
    	}

    	while(1) {
    		clear_buf(buf);
	    	read(pos_pipe, buf, 32);
	    	sscanf(buf, "%c %d %d %d %d", &id, &prev_x, &prev_y, &x, &y);
	    	if(coord[M - 1 - prev_y][prev_x] == (id + 1))
  				coord[M - 1 - prev_y][prev_x] = ' ';
  			coord[M - 1 - y][x] = id + 1;	//REDO: WORKS ONLY FOR FIRST 10 UNITS (0-9)!!!!
	    	clear_screen();
  			printf("%c[%d;%dH", 27, 0, 0);
  			for(i = 0; i < N; ++i) {
  				fprintf(stdout, "%s|\n", coord[i]);
  			}
  			for (i = 0; i <= M; ++i)
  			{
  				fprintf(stdout, "=");
  			}
  			printf("\n");
  			printf("%c[%d;%dH", 27, N+1, 0);

	  	}
  	}
}

void termination_handler(int signum) {	//A handler for child processes termination
	char buf[32];
	int id = 0;
	clear_buf(buf);
	read(term_id[0], buf, 32);
	id = atoi(buf);
	--active_num;
	alive[id] = 0;	//setting terminated unit as inactive
	fprintf(stdout, "INFO: Unit %d was terminated.\n", id + 1);	//"+1" is for users, who count from 1 :)
	free(fd[id]);
}

int main() {
	int i;
	int unit_num = 0;	//total number of ever created child-processes
	alive = (int*)malloc(unit_num * sizeof(int));
	fd = (int**)malloc(unit_num * sizeof(int*));
	for(i = 0; i < unit_num; i++)
		fd[i] = (int*)malloc(2 * sizeof(int));
	int *pos_pipe = (int*)malloc(unit_num * sizeof(int));	//a pipe for coordinates
	char buf[32];
	int cur_unit = 0;	//number of current child process being communicated with
	if(pipe(term_id) < 0) {	//creating a pipe for terminating units' IDs
		fprintf(stderr, "CRIT_ERR: Unable to create term. pipe!\n");
		exit(1);
	}
	if(pipe(pos_pipe) < 0) {	//creating a pipe for coordinates
		fprintf(stderr, "CRIT_ERR: Unable to create coord. pipe!\n");
		exit(1);
	}
	visualizer(pos_pipe[0]);	//creating visualizer process
	
	signal(SIGCHLD, termination_handler);	//setting a handler for child processes' termination
	fprintf(stdout, "INFO: Enter your commands below (print \"h\" to know more):\n");
	while(1) {	//main loop
		clear_buf(buf);
    	fscanf(stdin, "%s", buf);	//reading new command
    	if(strcmp(buf,"n") == 0) {	//adding new unit and pipe
    		++unit_num;
    		++active_num;  
    		alive = (int*)realloc(alive, unit_num * sizeof(int)); //allocating more memory for status flags
    		alive[unit_num - 1] = 1;
    		fd = (int**)realloc(fd, unit_num * sizeof(int*));	//allocaling more memory for the new fd
    		fd[unit_num - 1] = (int*)malloc(2 * sizeof(int));
    		if(pipe(fd[unit_num - 1]) < 0) {	//creating a new fd
				fprintf(stderr, "CRIT_ERR: Unable to create pipe!\n");
				exit(1);
			}
    		new_unit(fd[unit_num - 1][0], unit_num - 1, pos_pipe[1]);
    		fprintf(stdout, "INFO: New unit created (%d total).\n", active_num);
    		continue;
    	}
    	else if((cur_unit = atoi(buf) - 1) >= 0 && cur_unit < unit_num) {	//processing directives (numeration for users starts from 1)
    		if(alive[cur_unit] == 0) {
    			fprintf(stderr, "ERROR: This unit is inaccessible, because it was terminated.\n");
    			continue;
    		}
    		fprintf(stdout, "INFO: Enter directive for unit %d.\n", cur_unit + 1);	//"+1" is for users, who count from 1 :)
			clear_buf(buf);
    		read(0, buf, 32);	//recieving a command from keyboard
    		write(fd[cur_unit][1], buf, 32);	//sending a command to selected unit
    		continue;
    	}
    	else if(strcmp(buf, "s") == 0) {	//getting status information
    		fprintf(stdout, "INFO: Active units: ");
    		for(i = 0; i < unit_num; i++) {
    			if(alive[i] == 1)
    				fprintf(stdout, "%d ", i + 1);
    		}
    		fprintf(stdout, "(%d total).\n", active_num);
    		continue;
    	}
    	else if(strcmp(buf, "h") == 0) {	//showing help
    		fprintf(stdout, "INFO: Common commands:\n");
			fprintf(stdout, "1) n -- add new unit\n");
			fprintf(stdout, "2) s -- number of active units\n");
			fprintf(stdout, "3) N (ex. \"1\") -- enter a command for unit N\n");
			fprintf(stdout, "4) q -- quit the program\n");
			fprintf(stdout, "Unit directives:\n");
			fprintf(stdout, "1) u -- unit starts going up on N steps\n");
			fprintf(stdout, "2) d -- unit starts going down on N steps\n");
			fprintf(stdout, "3) l -- unit starts going left on N steps\n");
			fprintf(stdout, "4) r -- unit starts going right on N steps\n");
			fprintf(stdout, "5) p -- get unit's position\n");
			fprintf(stdout, "6) k -- destroy a unit\n");
    		continue;
    	}
    	else if(strcmp(buf, "q") == 0) {	//exiting the loop
    		for(i = 0; i < unit_num; i++) {
    			if(alive[i] == 1) {
    				write(fd[i][1], "k", 32);	//sending a command to selected unit
    				free(fd[i]);
    			}
    		}
    		free(alive);
    		free(fd);
    		fprintf(stdout, "INFO: Goodbye.\n");
    		exit(0);
    	}
    	fprintf(stderr, "ERROR: Unrecognized common command \"%s\".\n", buf);
	}
	return 0;
}