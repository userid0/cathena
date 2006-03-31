#include "map.h"

#define MAX_ADMINS	25 // [Zido]

extern short use_irc;

extern short irc_announce_flag;
extern short irc_announce_mvp_flag;
extern short irc_announce_shop_flag;
extern short irc_announce_jobchange_flag;

void irc_announce(char *buf);
void irc_announce_jobchange(struct map_session_data *sd);
void irc_announce_shop(struct map_session_data *sd,int flag);
void irc_announce_mvp(struct map_session_data *sd, struct mob_data *md);

int irc_parse(int fd);
void do_final_irc(void);
void do_init_irc(void);
void irc_send(char *buf);
void irc_parse_sub(int fd, char *incoming_string);
int send_to_parser(int fd, char *input,char key[2]);
struct IRC_Session_Info {
    int state;
    int fd;
    char username[30];
    char password[33];
};

/*struct IRC_Authed {
	char host[256];
};*/ //(WIP, Don't remove) [Zido]
typedef struct IRC_Session_Info IRC_SI;
