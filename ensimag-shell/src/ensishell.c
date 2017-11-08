/*****************************************************
 * Copyright Grégory Mounié 2008-2015                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <unistd.h>

#include <fcntl.h>

#include <sys/time.h>

#include "variante.h"
#include "readcmd.h"

#ifndef VARIANTE
#error "Variante non défini !!"
#endif

/* Guile (1.8 and 2.0) is auto-detected by cmake */
/* To disable Scheme interpreter (Guile support), comment the
 * following lines.  You may also have to comment related pkg-config
 * lines in CMakeLists.txt.
 */

#if USE_GUILE == 1
#include <libguile.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

#define COMMAND_JOBS "jobs"

typedef struct joblist joblist;
struct joblist
{
  pid_t pid;
  char *cmd;
  time_t debut;
  joblist *suiv;
};

joblist *jobslist = NULL;

void terminate(char *line); // prototype

// fonction appellée lorsqu'un processus se termine
static void ecoute_fils(int sig)
{
	pid_t pid;
	// on cherche quel fils a terminé
	while((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
		joblist *actuel = jobslist;
		joblist *prec = NULL;
		// on recherche les infos qu'on a sur lui dans notre jobslist
		while ((actuel != NULL) && (actuel->pid != pid)) {
			prec = actuel;
			actuel = actuel->suiv;
		}
		// s'il n'y est pas c'est que ce n'est pas un job (lancé avec &)
		if (actuel == NULL) {
			;
		// mais si on l'a trouvé
		} else {
			// on met à jour notre jobslist
			if (prec == NULL) {
				jobslist = jobslist->suiv;
			} else {
				prec->suiv = prec->suiv->suiv;
			}
			// et on affiche qu'il a terminé
			struct timeval tv;
			gettimeofday(&tv, NULL);
			int temps = tv.tv_sec - actuel->debut;
			printf("\n[SHELL-INFO] Le processus %d vient de terminer après %d secondes.\n", pid, temps);
			// avant de libérer la mémoire
			free(actuel->cmd);
			free(actuel);
		}
	}
}

// fonction qui affiche les jobs (lancés avec &) n'ayant pas encore terminé
void display_jobs()
{
	joblist *actuel = jobslist;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	// on itère sur tous les jobs
	while (actuel != NULL) {
		// et on affiche les infos
		printf("(%i) execute %s depuis %ld secondes\n", actuel->pid, actuel->cmd, tv.tv_sec - actuel->debut);
		actuel = actuel->suiv;
	}
}

// fonction qui ajoute à la jobslist un nouveau job (lancé avec &)
void add_job(pid_t pid, char *cmd)
{
	joblist *nouv = malloc(sizeof(joblist) * 1);
	nouv->pid = pid;
	char *cmd_copy = malloc(sizeof(char) * (strlen(cmd) + 1));
	nouv->cmd = strcpy(cmd_copy, cmd);
	struct timeval tv;
	gettimeofday(&tv, NULL);
	nouv->debut = tv.tv_sec;
	nouv->suiv = jobslist;
	jobslist = nouv;
}

// le coeur du shell, la fonction qui exécute une ligne de commande
int question6_executer(char *line)
{
	// on lit la ligne de commande saisie
	struct cmdline *cmd = parsecmd(&line);
	// en cas d'erreur on termine le programme
	if (!cmd) {
		terminate(0);
	}
	if (cmd->err) {
		printf("error: %s\n", cmd->err);
		return -1;
	}
	// puis on compte le nombre de commande spipelinées entre elles
	int nombre_commandes = 0;
	for (nombre_commandes = 0; cmd->seq[nombre_commandes] != NULL; nombre_commandes++);
	// et on crée les pipes qui vont les relier
	int pipe_in[2];
	int pipe_out[2];
	// et c'est parti : pour chaque commande
	for (int commande = 0; commande < nombre_commandes; commande++) {
		// on vérifie qu'il ne s'agit pas de "jobs" notre commande interne
		if (strncmp(cmd->seq[commande][0], COMMAND_JOBS, strlen(COMMAND_JOBS)) == 0) {
			// si c'est le cas alors on l'appelle
			display_jobs();
		// sinon on continue normalement
		} else {
			char *bin_path = "/bin/";
			// on crée le chemin de l'exécutable
			char *exec_path = calloc(sizeof(char), (strlen(cmd->seq[commande][0]) + strlen(bin_path) + 1));
			// qui débute par "/bin/"
			sprintf(exec_path, bin_path);
			// suivi du nom de la commande
			strcat(exec_path, cmd->seq[commande][0]);

			// s'il y a plus d'une commande, il y aura des pipes à créer
			if(nombre_commandes > 1) {
				// la première commande écrira seulement dans le pipe
				if(commande == 0) {
					pipe(pipe_out);
				// le dernière ne fera que lire depuis le pipe qu'on lui a laissé
				} else if (commande == nombre_commandes - 1) {
					pipe_in[PIPE_READ] = pipe_out[PIPE_READ];
					pipe_in[PIPE_WRITE] = pipe_out[PIPE_WRITE];
				// tandis que les commandes intermédiaires lisent depuis le pipe précédent
				// et écrivent dans le suivant
				} else {
					pipe_in[PIPE_READ] = pipe_out[PIPE_READ];
					pipe_in[PIPE_WRITE] = pipe_out[PIPE_WRITE];
					pipe(pipe_out);
				}
			}
			
			pid_t child_pid;
			// on fork ici !
			child_pid = fork();
			if (child_pid == 0) {
				// pour ce qui est du fils, on prépare les redirections
				int in_file, out_file;
				// la première commande lit, s'il existe, "cmd->in" en tant que "stdin"
				if (commande == 0 && cmd->in != NULL) {
					in_file = open(cmd->in, O_RDONLY);
					dup2(in_file, STDIN_FILENO);
					close(in_file);
				// ou bien la dernière lit, s'il existe, "cmd->out" en tant que "stdout"
				} else if (commande == nombre_commandes - 1 && cmd->out != NULL) {
					out_file = open(cmd->out, O_RDWR | O_CREAT, 0777);
					dup2(out_file, STDOUT_FILENO);
					close(out_file);
				}
				// et gère maintenant les substitutions d'entrée/sortie standards
				if(nombre_commandes > 1) {
					// la première commande va écrire dans son pipe
					if(commande == 0) {
						dup2(pipe_out[PIPE_WRITE], STDOUT_FILENO);
						close(pipe_out[PIPE_READ]);
						close(pipe_out[PIPE_WRITE]);
					// la dernière command eva lire depuis le pipe précédent
					} else if (commande == nombre_commandes - 1) {
						dup2(pipe_in[PIPE_READ], STDIN_FILENO);
						close(pipe_in[PIPE_READ]);
						close(pipe_in[PIPE_WRITE]);
					// et chaque commande entre lit depuis le précédent et écrit dans le suivant
					} else {
						dup2(pipe_in[PIPE_READ], STDIN_FILENO);
						close(pipe_in[PIPE_READ]);
						close(pipe_in[PIPE_WRITE]);
						dup2(pipe_out[PIPE_WRITE], STDOUT_FILENO);
						close(pipe_out[PIPE_READ]);
						close(pipe_out[PIPE_WRITE]);
					}
				}
				// enfin on lance l'exécution de la commande
				if (execvp(exec_path, cmd->seq[commande]) == -1) {
					perror("execvp: ");
				}
			// si on est le père
			} else {
				// et qu'il ne s'agit pas de la première commande
				if (nombre_commandes > 2 && commande > 0) {
					// alors on ferme le pipe de lecture
					close(pipe_in[PIPE_READ]);
					close(pipe_in[PIPE_WRITE]);
				// et s'il s'agit de la dernière
				} else if (nombre_commandes > 1 && commande == nombre_commandes - 1) {
					// alors on ferme le pipe d'écriture
					close(pipe_in[PIPE_READ]);
					close(pipe_in[PIPE_WRITE]);
				}
				// si la commande a été lancée sans &
				if (cmd->bg == 0 && commande == nombre_commandes - 1) {
					// alors on attend qu'elle termine
					waitpid(child_pid, NULL, 0);
				// sinon si elle a été lancée avec &
				} else if (cmd->bg != 0) {
					// alors on ajoute chaque commande à la jobslist
					add_job(child_pid, cmd->seq[commande][0]);
				}
			}
		}
	}
	free(cmd);
	return 0;
}

SCM executer_wrapper(SCM x)
{
        return scm_from_int(question6_executer(scm_to_locale_stringn(x, 0)));
}
#endif


void terminate(char *line) {
#if USE_GNU_READLINE == 1
	/* rl_clear_history() does not exist yet in centOS 6 */
	clear_history();
#endif
	if (line)
	  free(line);
	printf("exit\n");
	exit(0);
}


int main() {
        printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);
        
        // la terminaison d'un fils est prise en compte par la fonction "ecoute_fils"
        signal(SIGCHLD, ecoute_fils);

#if USE_GUILE == 1
        scm_init_guile();
        /* register "executer" function in scheme */
        scm_c_define_gsubr("executer", 1, 0, 0, executer_wrapper);
#endif

	while (1) {
		char *line=0;
		//int i, j;
		char *prompt = "ensishell>";

		/* Readline use some internal memory structure that
		   can not be cleaned at the end of the program. Thus
		   one memory leak per command seems unavoidable yet */
		line = readline(prompt);
		if (line == 0 || ! strncmp(line,"exit", 4)) {
			terminate(line);
		}

#if USE_GNU_READLINE == 1
		add_history(line);
#endif


#if USE_GUILE == 1
		/* The line is a scheme command */
		if (line[0] == '(') {
			char catchligne[strlen(line) + 256];
			sprintf(catchligne, "(catch #t (lambda () %s) (lambda (key . parameters) (display \"mauvaise expression/bug en scheme\n\")))", line);
			scm_eval_string(scm_from_locale_string(catchligne));
			free(line);
                        continue;
                }
#endif

		// on appelle simplement le traitant de commande
		question6_executer(line);
	}

}
