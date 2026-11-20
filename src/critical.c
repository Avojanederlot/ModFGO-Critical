#include <stdint.h>
#include <stddef.h>
extern int __android_log_print(int,const char*,const char*,...);
extern int pthread_create(unsigned long*,const void*,void*(*)(void*),void*);
extern int pthread_detach(unsigned long);
extern int pthread_key_create(unsigned int*,void(*)(void*));
extern void *pthread_getspecific(unsigned int);
extern int pthread_setspecific(unsigned int,const void*);
extern int usleep(unsigned int);
extern void *dlopen(const char*,int);
extern void *dlsym(void*,const char*);
extern char *dlerror(void);
extern int strcmp(const char*,const char*);
extern char *strstr(const char*,const char*);
struct info {uintptr_t base;const char *name;const void *phdr;uint16_t phnum;};
extern int dl_iterate_phdr(int(*)(struct info*,size_t,void*),void*);
static uintptr_t base;
static uintptr_t modbase;
static const char *path;
static int find_lib(struct info *i,size_t size,void *data){
 (void)size;(void)data;
 if(i->name && strstr(i->name,"/lib/arm64/libil2cpp.so")){base=i->base;path=i->name;}
 if(i->name && strstr(i->name,"/zygisk-ModFGO/"))modbase=i->base;
 return 0;
}
static void *resolve(void *h,const char *name){
 void *(*sym)(void*,const char*,size_t*)=(void*)(modbase+0x1e93ac);
 return sym(h,name,0);
}
#define LOG(...) __android_log_print(4,"ModFGO_Critical",__VA_ARGS__)
#define CONFIG "/sdcard/Android/data/com.bilibili.fgo.nearme.gamecenter/files/Mod/CriticalBeta.conf"
#define STATUS "/sdcard/Android/data/com.bilibili.fgo.nearme.gamecenter/files/Mod/CriticalBeta.status.json"
extern void *fopen(const char*,const char*);
extern char *fgets(char*,int,void*);
extern int fclose(void*);
extern int fprintf(void*,const char*,...);
extern long strtol(const char*, char**, int);
extern int rename(const char*,const char*);
extern int getpid(void);
static int enabled=1, installed=0;
static unsigned long calls,forced,enemy_skipped,critical_checks;
static unsigned long increment(unsigned long *p){
#if defined(__aarch64__)
 unsigned long old,next;unsigned int retry;
 __asm__ volatile("1: ldxr %0, [%3]\nadd %1, %0, #1\nstxr %w2, %1, [%3]\ncbnz %w2, 1b":"=&r"(old),"=&r"(next),"=&r"(retry):"r"(p):"memory");return old;
#else
 return __atomic_fetch_add(p,1,__ATOMIC_RELAXED);
#endif
}
static unsigned int context_key;
static int (*original_critical)(void*,void*);
static int critical_bridge(void *command,void *method){
 if(command&&__atomic_load_n(&enabled,__ATOMIC_RELAXED)&&pthread_getspecific(context_key)==command){increment(&critical_checks);return 1;}
 return original_critical(command,method);
}
static const char *stage="waiting";
static void read_config(void){void *f=fopen(CONFIG,"r");if(!f)return;char line[80];while(fgets(line,sizeof(line),f)){if(!strcmp(line,"enabled=0\n")||!strcmp(line,"enabled=0"))__atomic_store_n(&enabled,0,__ATOMIC_RELAXED);if(!strcmp(line,"enabled=1\n")||!strcmp(line,"enabled=1"))__atomic_store_n(&enabled,1,__ATOMIC_RELAXED);}fclose(f);}
static void status(void){void *f=fopen(STATUS ".tmp","w");if(!f)return;fprintf(f,"{\"version\":\"0.2.0-beta.7\",\"pid\":%d,\"stage\":\"%s\",\"installed\":%s,\"enabled\":%s,\"calls\":%lu,\"forced\":%lu,\"enemySkipped\":%lu,\"criticalChecks\":%lu}\n",getpid(),stage,installed?"true":"false",__atomic_load_n(&enabled,__ATOMIC_RELAXED)?"true":"false",__atomic_load_n(&calls,__ATOMIC_RELAXED),__atomic_load_n(&forced,__ATOMIC_RELAXED),__atomic_load_n(&enemy_skipped,__ATOMIC_RELAXED),__atomic_load_n(&critical_checks,__ATOMIC_RELAXED));fclose(f);rename(STATUS ".tmp",STATUS);}
typedef void *(*DamageFn)(void*,void*,void*,void*,void*,void*,void*);
static DamageFn original_damage;
static size_t enemy_offset,critical_offset;
static void *damage_bridge(void *self,void *attacker,void *defender,void *command,void *args,void *action,void *method){
 increment(&calls);
 void *previous=pthread_getspecific(context_key);
 unsigned char saved=0;int changed=0;
 pthread_setspecific(context_key,0);
 if(attacker&&command&&enemy_offset&&critical_offset){
  if(*((unsigned char*)attacker+enemy_offset))increment(&enemy_skipped);
  else if(__atomic_load_n(&enabled,__ATOMIC_RELAXED)){saved=*((unsigned char*)command+critical_offset);changed=1;*((unsigned char*)command+critical_offset)=1;pthread_setspecific(context_key,command);unsigned long n=increment(&forced);if(n<5)LOG("player critical forced command=%p",command);}
 }
 void *result=original_damage(self,attacker,defender,command,args,action,method);
 if(changed)*((unsigned char*)command+critical_offset)=saved;
 pthread_setspecific(context_key,previous);
 return result;
}
typedef void (*SetTimeScale)(float);
static SetTimeScale original_set_scale;
static int speed_override = 1;
static unsigned long redirected_speed_writes;

static float requested_scale(int request) {
 // This game build remaps an exact 2.0 to 4.0.
 return request == 2 ? 2.00001f : (float)request;
}

static void set_scale_bridge(float value) {
 int request = __atomic_load_n(&speed_override, __ATOMIC_RELAXED);
 if (request > 1 && value > 0.0f) {
  float target = requested_scale(request);
  if (value != target) {
   increment(&redirected_speed_writes);
   value = target;
  }
 }
 original_set_scale(value);
}

static int read_speed(void){
 const char *path="/sdcard/Android/data/com.bilibili.fgo.nearme.gamecenter/files/Mod/UnitySpeed.conf";
 void *file=fopen(path,"r");
 if(!file)return 1;
 char line[16],*end;
 int read_ok=fgets(line,sizeof(line),file)!=0;
 fclose(file);
 if(!read_ok||line[0]<'1'||line[0]>'9')return 1;
 long value=strtol(line,&end,10);
 return value>=1&&value<=30&&end[0]=='\n'&&end[1]=='\0'?(int)value:1;
}

/* Original ModFGO and the OPPO game build are both pinned by customize.sh. */
enum {
 DARK_CLOAK_ID = 960988,
 COMMON_QUEST_LOAD_RVA = 0x25389e4,
 MOD_QUEST_TRAMPOLINE_RVA = 0x236408,
 BATTLE_INFO_OFFSET = 80,
 USER_SERVANTS_OFFSET = 104,
 CLASS_PASSIVE_OFFSET = 456,
 MANAGED_ARRAY_DATA_OFFSET = 32,
 MAX_BATTLE_SERVANTS = 64,
 MAX_CLASS_PASSIVES = 128
};

typedef void (*QuestLoad)(void *, const void *);
static QuestLoad original_quest_load;
static void *passive_array_class;
static size_t (*managed_array_length)(void *);
static void *(*new_passive_array)(void *, size_t);
static void (*store_managed_reference)(void *, void **, void *);

static int ensure_dark_cloak(void *servant, int *was_empty) {
 void **field = (void **)((uint8_t *)servant + CLASS_PASSIVE_OFFSET);
 void *previous = *field;
 size_t count = previous ? managed_array_length(previous) : 0;
 if(count > MAX_CLASS_PASSIVES)return -1;
 const int32_t *old_ids = previous ? (const int32_t *)((uint8_t *)previous + MANAGED_ARRAY_DATA_OFFSET) : NULL;
 size_t matches = 0;
 for(size_t i = 0; i < count; i++)
  if(old_ids[i] == DARK_CLOAK_ID)matches++;
 if(matches == 1 && old_ids[0] == DARK_CLOAK_ID)return 0;

 void *replacement = new_passive_array(passive_array_class, count - matches + 1);
 if(!replacement)return -1;
 int32_t *new_ids = (int32_t *)((uint8_t *)replacement + MANAGED_ARRAY_DATA_OFFSET);
 /* The upstream wrapper writes index 0 after this function returns. */
 new_ids[0] = DARK_CLOAK_ID;
 size_t next = 1;
 for(size_t i = 0; i < count; i++)
  if(old_ids[i] != DARK_CLOAK_ID)new_ids[next++] = old_ids[i];
 store_managed_reference(servant, field, replacement);
 if(*field != replacement)return -1;
 if(!count)(*was_empty)++;
 return 1;
}

static void quest_load_with_dark_cloak(void *battle, const void *method) {
 QuestLoad load = __atomic_load_n(&original_quest_load, __ATOMIC_ACQUIRE);
 load(battle, method);
 if(!battle)return;
 void *info = *(void **)((uint8_t *)battle + BATTLE_INFO_OFFSET);
 void *servants = info ? *(void **)((uint8_t *)info + USER_SERVANTS_OFFSET) : NULL;
 size_t count = servants ? managed_array_length(servants) : 0;
 if(!count || count > MAX_BATTLE_SERVANTS){
  LOG("Dark Cloak skipped: unexpected userSvt count=%zu",count);
  return;
 }
 void **entries = (void **)((uint8_t *)servants + MANAGED_ARRAY_DATA_OFFSET);
 int updated = 0, empty = 0, already_present = 0, failed = 0;
 for(size_t i = 0; i < count; i++){
  if(!entries[i])continue;
  int result = ensure_dark_cloak(entries[i], &empty);
  if(result > 0)updated++;
  else if(result == 0)already_present++;
  else failed++;
 }
 LOG("Dark Cloak skill=%d updated=%d empty=%d already=%d failed=%d",
     DARK_CLOAK_ID, updated, empty, already_present, failed);
}

static void *typed_field(void *klass, const char *field_name, const char *expected,
                         size_t expected_offset, void *h) {
 if(!klass)return NULL;
 void *(*get_field)(void *, const char *) = resolve(h, "il2cpp_class_get_field_from_name");
 size_t (*offset)(void *) = resolve(h, "il2cpp_field_get_offset");
 void *(*field_type)(void *) = resolve(h, "il2cpp_field_get_type");
 const char *(*type_name)(void *) = resolve(h, "il2cpp_type_get_name");
 void *(*type_class)(void *) = resolve(h, "il2cpp_class_from_type");
 if(!get_field || !offset || !field_type || !type_name || !type_class)return NULL;
 void *field = get_field(klass, field_name);
 if(!field || offset(field) != expected_offset)return NULL;
 void *type = field_type(field);
 const char *actual = type ? type_name(type) : NULL;
 return actual && !strcmp(actual, expected) ? type_class(type) : NULL;
}

static int prepare_dark_cloak(void *h, void *battle_class) {
 void *(*array_element_class)(void *) = resolve(h, "il2cpp_class_get_element_class");
 uint32_t (*array_header_size)(void) = resolve(h, "il2cpp_array_object_header_size");
 managed_array_length = resolve(h, "il2cpp_array_length");
 new_passive_array = resolve(h, "il2cpp_array_new_specific");
 store_managed_reference = resolve(h, "il2cpp_gc_wbarrier_set_field");
 if(!array_element_class || !managed_array_length || !new_passive_array || !store_managed_reference)
  return 0;
 if(array_header_size && array_header_size() != MANAGED_ARRAY_DATA_OFFSET)return 0;

 void *info_class = typed_field(battle_class, "battle_info", "BattleInfoData", BATTLE_INFO_OFFSET, h);
 void *servants_class = typed_field(info_class, "userSvt", "BattleUserServantData[]", USER_SERVANTS_OFFSET, h);
 void *servant_class = servants_class ? array_element_class(servants_class) : NULL;
 passive_array_class = typed_field(servant_class, "classPassive", "System.Int32[]", CLASS_PASSIVE_OFFSET, h);
 return passive_array_class != NULL;
}

static int connect_dark_cloak(void) {
 QuestLoad *slot = (QuestLoad *)(modbase + MOD_QUEST_TRAMPOLINE_RVA);
 QuestLoad current = __atomic_load_n(slot, __ATOMIC_ACQUIRE);
 if(!current || current == quest_load_with_dark_cloak ||
    (uintptr_t)current == base + COMMON_QUEST_LOAD_RVA)return 0;

 /* The main module installs this trampoline once. Reuse its wrapper instead
    of installing a second inline hook on the same IL2CPP method. */
 __atomic_store_n(&original_quest_load, current, __ATOMIC_RELEASE);
 __atomic_store_n(slot, quest_load_with_dark_cloak, __ATOMIC_RELEASE);
 LOG("Dark Cloak ready: skill=%d", DARK_CLOAK_ID);
 return 1;
}

static void *observe(void *unused){
 (void)unused; usleep(3000000);
 for(unsigned i=0;i<600;i++){
  dl_iterate_phdr(find_lib,0);
  if(base){uintptr_t *v=(uintptr_t*)(base+0x5de13a0);if(v[0]&&v[1]>v[0]&&(v[1]-v[0])<16000)break;}
  usleep(100000);
 }
 if(!base||!modbase){stage="library_timeout";status();LOG("library not loaded");return 0;}
 uintptr_t *vec=(uintptr_t*)(base+0x5de13a0);
 size_t n=(vec[1]-vec[0])/8;
 if(!vec[0]||!n||n>2000){stage="assemblies_not_ready";status();LOG("assembly vector not ready");return 0;}
 void *(*xopen)(const char*,int)=(void*)(modbase+0x1ec8b4);
 void *h=xopen("libil2cpp.so",0);
 if(!h){stage="library_open_failed";status();LOG("xdl open failed");return 0;}

 void (*hook)(void*,void*,void**)=(void*)(modbase+0x17941c);

 void *(*domain_get)(void)=resolve(h,"il2cpp_domain_get");
 void *(*thread_attach)(void*)=resolve(h,"il2cpp_thread_attach");
 void *(*assembly_image)(void*)=resolve(h,"il2cpp_assembly_get_image");
 const char *(*image_name)(void*)=resolve(h,"il2cpp_image_get_name");
 size_t (*class_count)(void*)=resolve(h,"il2cpp_image_get_class_count");
 void *(*image_class)(void*,size_t)=resolve(h,"il2cpp_image_get_class");
 const char *(*class_name)(void*)=resolve(h,"il2cpp_class_get_name");
 void *(*class_methods)(void*,void**)=resolve(h,"il2cpp_class_get_methods");
 const char *(*method_name)(void*)=resolve(h,"il2cpp_method_get_name");
 uint32_t (*param_count)(void*)=resolve(h,"il2cpp_method_get_param_count");
 void *(*method_param)(void*,uint32_t)=resolve(h,"il2cpp_method_get_param");
 const char *(*type_name)(void*)=resolve(h,"il2cpp_type_get_name");
 void *(*return_type)(void*)=resolve(h,"il2cpp_method_get_return_type");
 uint32_t (*method_flags)(void*,uint32_t*)=resolve(h,"il2cpp_method_get_flags");
 void *(*class_from_type)(void*)=resolve(h,"il2cpp_class_from_type");
 int (*is_valuetype)(void*)=resolve(h,"il2cpp_class_is_valuetype");
 void *(*get_field)(void*,const char*)=resolve(h,"il2cpp_class_get_field_from_name");
 size_t (*field_offset)(void*)=resolve(h,"il2cpp_field_get_offset");
 if(!domain_get||!thread_attach||!assembly_image||!image_name||!class_count||!image_class||!class_name||!class_methods||!method_name||!param_count||!method_param||!type_name||!return_type){stage="missing_export";status();LOG("required export missing");return 0;}
 thread_attach(domain_get());
 read_config();
 LOG("native observer ready base=%p assemblies=%lu",(void*)base,(unsigned long)n);
 void **assemblies=(void**)vec[0];
 void *damage_target=0,*critical_target=0,*quest_owner=0;
 for(size_t i=0;i<n;i++){
  void *image=assembly_image(assemblies[i]);const char *iname=image_name(image);
  if(!iname||strcmp(iname,"Assembly-CSharp.dll"))continue;
  size_t count=class_count(image);
  LOG("image %s classes=%lu",iname,(unsigned long)count);
  for(size_t j=0;j<count;j++){
   void *klass=image_class(image,j);const char *name=class_name(klass);
   if(name&&get_field&&field_offset&&!strcmp(name,"BattleServantData")){void *f=get_field(klass,"isEnemy");if(f)enemy_offset=field_offset(f);LOG("FIELD BattleServantData.isEnemy=%lx",(unsigned long)enemy_offset);}
   if(name&&get_field&&field_offset&&!strcmp(name,"BattleCommandData")){void *f=get_field(klass,"critical");if(f)critical_offset=field_offset(f);LOG("FIELD BattleCommandData.critical=%lx",(unsigned long)critical_offset);}
   if(name && method_flags && !strcmp(name,"BattleData")){
    void *iter = NULL, *method;
    while((method = class_methods(klass, &iter))){
     const char *label = method_name(method);
     if(!label || strcmp(label, "commonQuestLoad") || param_count(method) != 0 ||
        (uintptr_t)*(void **)method != base + COMMON_QUEST_LOAD_RVA)continue;
     uint32_t implementation_flags = 0;
     if(!(method_flags(method, &implementation_flags) & 0x10) &&
        !strcmp(type_name(return_type(method)), "System.Void"))quest_owner = klass;
    }
   }
   if(!name||(strcmp(name,"BattleLogic")&&strcmp(name,"BattleCommandData")))continue;
   void *it=0,*method;unsigned total=0;
   while((method=class_methods(klass,&it))){
    const char *mn=method_name(method);unsigned pn=param_count(method);total++;
    if(!mn)continue;
    if(!strcmp(name,"BattleCommandData")&&pn==0&&((uintptr_t)*(void**)method-base)==0x25fb4a8&&!strcmp(type_name(return_type(method)),"System.Boolean")){critical_target=*(void**)method;LOG("critical getter matched %s",mn);}
    if(!strcmp(mn,"GetDamageList")&&pn==5&&method_flags&&class_from_type&&is_valuetype&&((uintptr_t)*(void**)method-base)==0x2638474){
     const char *expected[]={"BattleServantData","BattleServantData","BattleCommandData","BattleLogic.DamageProcessArgs","BattleActionData"};
     int valid=!strcmp(type_name(return_type(method)),"BattleActionData.DamageData");
     for(unsigned k=0;k<5;k++)if(strcmp(type_name(method_param(method,k)),expected[k]))valid=0;
     uint32_t f=0;if(valid&&!(method_flags(method,&f)&0x10)&&!is_valuetype(class_from_type(return_type(method))))damage_target=*(void**)method;
    }

   }
   LOG("BattleLogic total=%u",total);
  }
 }
 if(damage_target&&critical_target&&enemy_offset==0x203&&critical_offset==0x44&&!pthread_key_create(&context_key,0)){hook(critical_target,critical_bridge,(void**)&original_critical);if(original_critical)hook(damage_target,damage_bridge,(void**)&original_damage);installed=original_damage!=0&&original_critical!=0;stage=installed?"ready":"hook_failed";LOG("critical beta hook installed=%d",installed);}else {stage="unsupported";LOG("damage bridge rejected by version checks");}
 void *(*resolve_icall)(const char*)=resolve(h,"il2cpp_resolve_icall");
 float (*get_scale)(void)=0;
 SetTimeScale set_scale=0;
 int setter_hook_attempted=0;
 unsigned long fallback_repairs=0;
 unsigned long reported_writes=0;
 unsigned long reported_repairs=0;
 int request=1;
 int applied_request=1;
 unsigned retry_ticks=0;
 unsigned tick=0;
 int cloak_waiting = quest_owner && prepare_dark_cloak(h, quest_owner);
 if(!cloak_waiting)LOG("Dark Cloak unavailable: quest metadata or IL2CPP API mismatch");
 for(;;){
  if(tick<12 || (tick-12)%20==0){
   if(cloak_waiting && connect_dark_cloak())cloak_waiting = 0;
   if(get_scale && set_scale){
    int next=read_speed();
    if(next!=request)retry_ticks=0;
    request=next;
    __atomic_store_n(&speed_override, request, __ATOMIC_RELAXED);
   }
   if(tick>=12){
    unsigned long writes=__atomic_load_n(&redirected_speed_writes, __ATOMIC_RELAXED);
    if(writes!=reported_writes || fallback_repairs!=reported_repairs){
     LOG("Unity speed writes redirected=%lu pollingRepairs=%lu request=%d",
         writes, fallback_repairs, request);
     reported_writes=writes;
     reported_repairs=fallback_repairs;
    }
   }
   read_config();
   status();
  }
  if(tick==12 && resolve_icall){
   get_scale=resolve_icall("UnityEngine.Time::get_timeScale()");
   set_scale=resolve_icall("UnityEngine.Time::set_timeScale(System.Single)");
   LOG("Unity speed controller ready=%d",get_scale!=0 && set_scale!=0);
  }
  if(request>1 && get_scale && set_scale && installed && !setter_hook_attempted){
   setter_hook_attempted=1;
   hook((void*)set_scale, (void*)set_scale_bridge, (void**)&original_set_scale);
   if(original_set_scale)set_scale=original_set_scale;
   LOG("Unity speed setter hook installed=%d", original_set_scale!=0);
  }
  if(retry_ticks)retry_ticks--;
  if(get_scale && set_scale && !retry_ticks &&
     (request!=1 || applied_request!=1)){
   float current=get_scale();
   float delta=current-request;
   // A zero time scale is an intentional pause. Resume when Unity unpauses.
   if(current>0){
    if(delta>0.0001f || delta<-0.0001f){
     if(applied_request==request)fallback_repairs++;
     set_scale(requested_scale(request));
     float actual=get_scale();
     delta=actual-request;
     if(!(actual>0) || delta>0.0001f || delta<-0.0001f){
      retry_ticks=5;
     }else if(applied_request!=request){
      LOG("Unity speed requested=%d actual=%.6f",request,(double)actual);
      applied_request=request;
     }
    }else{
     applied_request=request;
    }
   }
  }
  tick++;
  usleep(tick<=12 ? 1000000 : 50000);
 }
 return 0;
}
#ifndef CRITICAL_TEST
__attribute__((constructor)) static void init(void){unsigned long t;if(!pthread_create(&t,0,observe,0))pthread_detach(t);}
#endif
