#include "greatest.h"
#include "sc_log.h"
#include <string.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
typedef struct { volatile int ready; volatile int received; char payload[1024]; } sc_netlog_state_t;
static DWORD WINAPI sc_netlog_server_thread(LPVOID ctx){ sc_netlog_state_t *st=(sc_netlog_state_t*)ctx; SOCKET srv=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP); if(srv==INVALID_SOCKET) return 0; struct sockaddr_in addr; memset(&addr,0,sizeof(addr)); addr.sin_family=AF_INET; addr.sin_port=htons(19091); addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK); if(bind(srv,(struct sockaddr*)&addr,sizeof(addr))!=0){closesocket(srv);return 0;} if(listen(srv,1)!=0){closesocket(srv);return 0;} st->ready=1; SOCKET cli=accept(srv,NULL,NULL); if(cli!=INVALID_SOCKET){ int n=recv(cli,st->payload,(int)sizeof(st->payload)-1,0); if(n>0){st->payload[n]=0; st->received=1;} closesocket(cli);} closesocket(srv); return 0; }
#endif
TEST log_smoke(void){ ASSERT_EQ(0, log_init(LOG_CONSOLE, LOG_DEBUG, NULL)); log_message(LOG_INFO, "log smoke"); log_close(); PASS(); }
TEST log_network_fallback_to_file(void){
    const char *fallback_path = "sc_log_fallback_test.log";
    remove(fallback_path);
    ASSERT_EQ(0, log_init(LOG_NETWORK, LOG_DEBUG, "127.0.0.1:19099"));
    ASSERT_EQ(0, log_set_fallback(LOG_FILE, fallback_path));
    log_message(LOG_ERROR, "FALLBACK_TOKEN");
    log_close();

    FILE *fp = fopen(fallback_path, "r");
    ASSERT(fp != NULL);
    char buf[1024];
    size_t n = fread(buf, 1, sizeof(buf) - 1, fp);
    buf[n] = '\0';
    fclose(fp);
    ASSERT(strstr(buf, "FALLBACK_TOKEN") != NULL);
    remove(fallback_path);
    PASS();
}
TEST log_network_localhost(void){
#ifdef _WIN32
WSADATA wsa; ASSERT_EQ(0, WSAStartup(MAKEWORD(2,2), &wsa)); sc_netlog_state_t st; memset(&st,0,sizeof(st)); HANDLE thr=CreateThread(NULL,0,sc_netlog_server_thread,&st,0,NULL); ASSERT(thr!=NULL); for(int i=0;i<50 && !st.ready;++i) Sleep(10); ASSERT(st.ready==1); ASSERT_EQ(0, log_init(LOG_NETWORK, LOG_DEBUG, "127.0.0.1:19091")); log_message(LOG_ERROR, "NETLOG_TOKEN"); log_close(); WaitForSingleObject(thr,2000); CloseHandle(thr); WSACleanup(); ASSERT(st.received==1); ASSERT(strstr(st.payload, "NETLOG_TOKEN")!=NULL);
#endif
PASS(); }
SUITE(sc_log_suite){ RUN_TEST(log_smoke); RUN_TEST(log_network_fallback_to_file); RUN_TEST(log_network_localhost);}
