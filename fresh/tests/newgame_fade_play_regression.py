"""Exercise real NewGame menu/video/loading/fade input at normal clock speed.

Run explicitly with an isolated game root and no existing rebuilt play window.
Uses original assets, real window messages and numeric snapshots; never invokes
story callbacks, writes game state, seeds RNG or injects the rebuilt process.
"""
import argparse,ctypes,json,os,secrets,subprocess,time
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--exe',required=True,type=Path)
p.add_argument('--game-root',required=True,type=Path)
p.add_argument('--output',required=True,type=Path)
p.add_argument('--expect-stalled',action='store_true',help='Verify a retained pre-fix binary reproduces the stalled overlay')
p.add_argument('--after-archive-return',action='store_true',help='First load slot0 and return to Logo through actual F7/4/Yes input')
args=p.parse_args()
root=Path('H:/PAL4/objs');game=args.game_root.resolve();out=args.output.resolve()
if not out.is_relative_to(root.resolve()):p.error('test artifacts must stay below H:/PAL4/objs')
if not (game/'config.cfg').is_file():p.error('an isolated original-resource game root is required')
out.mkdir(exist_ok=False)
existing=ctypes.WinDLL('user32',use_last_error=True)
existing.FindWindowA.argtypes=[ctypes.c_char_p,ctypes.c_char_p]
existing.FindWindowA.restype=ctypes.c_void_p
if existing.FindWindowA(b'rwc_rebuilt',b'PAL4-Rebuilt'):
    raise RuntimeError('close the existing rebuilt play window before this isolated regression')
env={k:v for k,v in os.environ.items() if not k.startswith('PAL4_COMPARE_')}
env.update(PAL4_COMPARE_SCRIPTED_INPUT='1',PAL4_COMPARE_STATE_DIR=str(out),PAL4_COMPARE_NONACTIVATING='1',TEMP=str(root/'tmp'),TMP=str(root/'tmp'))
stdout=(out/'game.log').open('w',encoding='utf-8');stderr=(out/'error.log').open('w',encoding='utf-8')
proc=subprocess.Popen([str(args.exe),str(game/'config.cfg'),'--play'],cwd=game,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
u=ctypes.WinDLL('user32',use_last_error=True)
u.GetWindowThreadProcessId.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_ulong)]
u.GetClassNameA.argtypes=[ctypes.c_void_p,ctypes.c_char_p,ctypes.c_int]
u.PostMessageW.argtypes=[ctypes.c_void_p,ctypes.c_uint,ctypes.c_size_t,ctypes.c_ssize_t]
u.SetWindowPos.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_uint]
EC=ctypes.WINFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_ssize_t);u.EnumWindows.argtypes=[EC,ctypes.c_ssize_t]
events=[];hwnd=None
@EC
def enum(h,data):
 global hwnd
 pid=ctypes.c_ulong();u.GetWindowThreadProcessId(h,ctypes.byref(pid))
 name=ctypes.create_string_buffer(64);u.GetClassNameA(h,name,64)
 if pid.value==proc.pid and name.value==b'rwc_rebuilt':hwnd=h
 return 1

def check():
 if proc.poll() is not None:raise RuntimeError('game exited: '+(out/'error.log').read_text(errors='replace')[-1500:])
def post(msg,w=0,l=0):
 check();assert u.PostMessageW(hwnd,msg,w,l)
 events.append(dict(time=time.time(),message=msg,wparam=w,lparam=l));(out/'input.json').write_text(json.dumps(events,indent=2))
def key(code,scan):
 post(0x100,code,1|(scan<<16));time.sleep(.06);post(0x101,code,0xc0000001|(scan<<16))
def snapshot():
 request=secrets.randbelow(0x7ffffffe)+1;path=out/f'compare-native-{proc.pid}-{request}.json';post(0x8504,request,2)
 stop=time.monotonic()+12
 while not path.exists():
  check()
  if time.monotonic()>stop:raise TimeoutError('snapshot pending; do not replay request '+str(request))
  time.sleep(.02)
 return json.loads(path.read_text(encoding='utf-8-sig'))
try:
 stop=time.monotonic()+45
 while not hwnd or 'Persistent Logo state0 cold entry' not in (out/'game.log').read_text(errors='replace'):
  check();u.EnumWindows(enum,0)
  if time.monotonic()>stop:raise TimeoutError('menu not ready')
  time.sleep(.1)
 # The known 1600x900 nonprimary display is selected by the runtime. Place the
 # client inside its actual current bounds using EnumDisplayMonitors.
 class RECT(ctypes.Structure):_fields_=[('l',ctypes.c_long),('t',ctypes.c_long),('r',ctypes.c_long),('b',ctypes.c_long)]
 class MON(ctypes.Structure):_fields_=[('size',ctypes.c_ulong),('rect',RECT),('work',RECT),('flags',ctypes.c_ulong),('name',ctypes.c_wchar*32)]
 MC=ctypes.WINFUNCTYPE(ctypes.c_int,ctypes.c_void_p,ctypes.c_void_p,ctypes.POINTER(RECT),ctypes.c_ssize_t)
 u.GetMonitorInfoW.argtypes=[ctypes.c_void_p,ctypes.POINTER(MON)];u.EnumDisplayMonitors.argtypes=[ctypes.c_void_p,ctypes.c_void_p,MC,ctypes.c_ssize_t]
 monitors=[]
 @MC
 def monitor(h,dc,rect,param):
  m=MON();m.size=ctypes.sizeof(m)
  if u.GetMonitorInfoW(h,ctypes.byref(m)) and not m.flags&1 and (m.rect.r-m.rect.l,m.rect.b-m.rect.t)==(1600,900):monitors.append((m.rect.l,m.rect.t,m.name))
  return 1
 u.EnumDisplayMonitors(None,None,monitor,0);assert len(monitors)==1
 left,top,display=monitors[0];assert u.SetWindowPos(hwnd,None,left+30,top+100,0,0,0x15)
 if args.after_archive_return:
  def click(x,y):
   point=x|(y<<16);post(0x200,0,point);post(0x201,1,point);time.sleep(.08);post(0x202,0,point);time.sleep(.2)
  click(122,500);click(110,540);click(250,125);key(13,28)
  stop=time.monotonic()+30
  while time.monotonic()<stop:
   loaded=snapshot()
   if loaded['world_state_active'] and loaded['scripts']['arena_ready']:break
   time.sleep(.2)
  else:raise TimeoutError('slot0 did not restore live Scene')
  (out/'archive-scene.json').write_text(json.dumps(loaded,indent=2))
  key(118,65);time.sleep(.3);key(27,1);key(52,5);time.sleep(.3);key(13,28)
  stop=time.monotonic()+10
  while time.monotonic()<stop:
   logo=snapshot()
   if logo['scripts']['logo_active']:break
   time.sleep(.2)
  else:raise TimeoutError('normal System quit did not return to Logo')
  assert logo['scripts']['scene_world']==0 and not logo['scripts']['scene_active']
  (out/'returned-logo.json').write_text(json.dumps(logo,indent=2))
 point=122|(405<<16);post(0x200,0,point);post(0x201,1,point);time.sleep(.08);post(0x202,0,point)
 stop=time.monotonic()+60
 while True:
  check()
  opening=snapshot()['scripts']
  if opening['world']=='Q01' and opening['scene']=='N01' and opening['arena_ready']:break
  if time.monotonic()>stop:raise TimeoutError('opening videos did not reach N01')
  key(27,1)
  time.sleep(.4)
 # Resource assembly can still be in progress. A queued numeric snapshot runs
 # only when the real game message/frame loop resumes.
 samples=[];stop=time.monotonic()+18;first=None
 while time.monotonic()<stop:
  s=snapshot();state=s['scripts'];samples.append(s)
  if state['scene']=='N01' and state['arena_ready'] and state['fade_duration']==2:
   if first is None:first=time.monotonic()
   if time.monotonic()-first>3:break
  time.sleep(.2)
 else:raise TimeoutError('opening fade not observed')
 states=[s['scripts'] for s in samples if s['scripts']['scene']=='N01' and s['scripts']['fade_duration']==2]
 assert len(states)>=2
 verdict=dict(pid=proc.pid,display=display,normal_clock=True,samples=len(states),first=states[0],last=states[-1],expected_stalled=args.expect_stalled)
 if args.expect_stalled:
  assert all(x['fade_elapsed']==0 and x['fade_alpha']==255 and x['fade_active']==1 for x in states),states[-1]
 else:
  assert states[-1]['fade_elapsed']>=2 and states[-1]['fade_alpha']==0 and states[-1]['fade_active']==0,states[-1]
  assert states[-1]['fade_draw_calls']>0
 verdict['passed']=True;(out/'verdict.json').write_text(json.dumps(verdict,indent=2));print(json.dumps(verdict,indent=2))
finally:
 if hwnd and proc.poll() is None:
  post(0x10)
  if proc.wait(timeout=25)!=0:raise RuntimeError('game failed during normal shutdown')
 stdout.close();stderr.close()
