/* syslogd.c */
int main(int argc, char *argv[]);
void Init(int argc, char *argv[]);
void ReadConfigFile(void);
void DoMessage(struct message *m);
void FindActions(struct message *m);
void DoActions(struct actions *actions, struct message *m);
void SelectActions(struct action *actions, struct message *m);
void DoFileAction(struct action *action, struct message *m);
void ReopenFiles(void);
void SyncFiles(void);
void FlushMessages(void);

/* conf-misc.c */
void SetDirectory(const char *argument);
void SetSyslogPort(char *name);
void SetMarkInterval(char *argument);
void SetRepeatInterval(char *argument);
void SetInetForwardFrom(const char *name);
void SetInetReceiveFrom(const char *name);
void SetSimpleHostnames(const char *name);
void SetStripDomains(const char *name);
struct aclEntry *CreateHostACL(const char *hostName);
struct aclEntry *CreateDomainACL(char *domainName);
struct aclEntry *CreateNetworkACL(int i1, int i2, int i3, int i4, int bits);
struct aclList *AppendACL(struct aclList *list, struct aclEntry *acl);
void MakeACL(char *name, struct aclList *list);
void MakeFacility(int bitmap, struct program *programs);
struct facility *AppendFacility(struct facility *list, struct facility *new);
struct programNames *MakeProgramName(const char *name);
struct programNames *AppendProgramName(struct programNames *list, struct programNames *new);
struct priority *MakePriority(enum comparisons comparison, int priority, struct actions *actions);
struct priority *AppendPriority(struct priority *list, struct priority *new);
struct actions *MakeActions(struct action *actions);
struct action *MakeAction(enum actionTypes type, const char *dest);
struct action *AppendAction(struct action *list, struct action *new);
struct program *MakeProgram(struct programNames *programNames, struct priority *priorities);
struct program *AppendProgram(struct program *list, struct program *new);
void InitActionOptions(void);

/* debug.c */
#if defined(DEBUG)
void DebugInit(int count);
void DebugParse(char *debug);
void DebugPrintf(int facility, int level, char *fmt, ...);
#endif

/* local.c */
int InitLocal(const char *path);
int InitKlog(const char *path);
void GetStreamsMessage(void);
void GetFIFOMessage(void);
void GetDgramMessage(void);
void OpenNewSocket(fd_set *vector);
void GetSocketMessage(int fd, fd_set *vector);
void GetKlogMessage(void);
void AccumulateMessage(int fd, char *message, int size);

/* inet.c */
int InitInet(void);
void GetInetMessage(void);
void DoInetMessage(char *message, int length, char *host, struct in_addr address);
bool SimpleHostname(char *host, struct in_addr address);
void StripDomains(char *host, struct in_addr address);
struct hash *CheckCache(struct hashTable *table, struct in_addr address);
char *GetHostName(struct in_addr address);
bool CheckACL(struct aclList *acl, struct in_addr address);
bool CheckNetworkACL(struct aclNetwork acl, struct in_addr address);
bool CheckDomainACL(char *domain, struct in_addr address);
bool CheckHostACL(char *pattern, struct in_addr address);
void TestACLs(struct in_addr address);
void DoForwardAction(struct action *action, struct message *m);
unsigned short GetPortByName(char *name);
/* misc.c */

void ErrorMessage(const char *message, ...);
void WarningMessage(const char *message, ...);
void InfoMessage(const char *message, ...);
void FatalError(const char *message, ...);
void LogStatistics(void);
void ResetStatistics(void);
void Signal(int sig);
void Exit(void);
void Usage(void);
int LookupAction(const char *action);
int LookupFacility(const char *facility);
int LookupPriority(const char *priority);
struct acls *LookupACL(const char *name);
bool ValidTimestamp(const char *ts);
struct hashTable *InitCache(void);
void CheckCacheSize(struct hashTable *table);
