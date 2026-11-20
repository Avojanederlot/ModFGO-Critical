#define CRITICAL_TEST
#include "critical.c"
extern int puts(const char*);
static void *context;
void *pthread_getspecific(unsigned int key){(void)key;return context;}
int pthread_setspecific(unsigned int key,const void *value){(void)key;context=(void*)value;return 0;}
int __android_log_print(int p,const char *tag,const char *fmt,...){(void)p;(void)tag;(void)fmt;return 0;}
static void *expected[7];static int expected_result,failed;
static int getter(void *cmd,void *method){(void)method;return *((unsigned char*)cmd+critical_offset);}
static void *original(void *a,void*b,void*c,void*d,void*e,void*f,void*g){
 void *args[]={a,b,c,d,e,f,g};for(int i=0;i<7;i++)if(args[i]!=expected[i])failed++;
 if(d&&critical_bridge(d,g)!=expected_result)failed++;
 return (void*)0x12345678;
}
static void run(unsigned char enemy,int on,unsigned char natural,int want){
 unsigned char attacker[0x220]={0},command[0x80]={0};attacker[0x203]=enemy;command[0x44]=natural;
 enabled=on;expected_result=want;void*saved=(void*)0x2345;context=saved;
 void *args[]={(void*)1,attacker,(void*)3,command,(void*)5,(void*)6,(void*)7};
 for(int i=0;i<7;i++)expected[i]=args[i];
 if(damage_bridge(args[0],args[1],args[2],args[3],args[4],args[5],args[6])!=(void*)0x12345678)failed++;
 if(command[0x44]!=natural||context!=saved)failed++;
}
int main(void){enemy_offset=0x203;critical_offset=0x44;original_damage=original;original_critical=getter;
 run(0,1,0,1);run(0,0,0,0);run(0,0,1,1);run(1,1,0,0);run(1,1,1,1);
 if(calls!=5||forced!=1||enemy_skipped!=2||critical_checks!=1)failed++;
 if(failed){puts("critical adapter FAIL");return 1;}puts("critical adapter: 5 cases PASS (7 arguments, return, player/enemy, toggle, field/TLS restoration)");return 0;}
