"""Build with local Zig and operator-provided Android arm64 system libraries.
python tools/build.py --zig /path/to/zig --lib-dir /path/to/android-libs
With no build arguments, validate and package the existing native binary.
"""
import argparse,hashlib,json,pathlib,subprocess,zipfile
root=pathlib.Path(__file__).resolve().parents[1]
ap=argparse.ArgumentParser();ap.add_argument('--zig');ap.add_argument('--lib-dir');args=ap.parse_args()
module=root/'module';payload=module/'payload/arm64-v8a/libfgomod_payload.so'
if bool(args.zig)!=bool(args.lib_dir):ap.error('--zig and --lib-dir must be used together')
if args.zig:
 libs=pathlib.Path(args.lib_dir)
 subprocess.run([args.zig,'cc','-target','aarch64-linux-android','-shared','-fPIC','-nostdlib','-fno-stack-protector','-fno-builtin','-g0','-O2','-Wl,-z,defs',str(root/'src/critical.c'),*(str(libs/x) for x in ['libc.so','libdl.so','liblog.so']),'-o',str(payload)],check=True)
header=payload.read_bytes()[:20]
assert header[:5]==b'\x7fELF\x02' and int.from_bytes(header[18:20],'little')==183,'arm64 ELF payload required'
dist=root/'dist';dist.mkdir(exist_ok=True)
archive=dist/'ModFGO-Critical-0.2.0-beta.7.zip'
manifest={}
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
 for p in sorted(module.rglob('*')):
  if p.is_file():
   relative=p.relative_to(module).as_posix();data=p.read_bytes()
   if p.suffix in ('.sh','.prop','.html','.css','.js'):assert b'\r\n' not in data,relative+' requires LF'
   z.writestr(relative,data);manifest[relative]=hashlib.sha256(data).hexdigest()
 for name in ['README.md','VALIDATION.md','THIRD_PARTY.md','RELEASE_NOTES.md']:z.write(root/name,'docs/'+name)
with zipfile.ZipFile(archive) as z:assert z.testzip() is None
(dist/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
(dist/'SHA256SUMS.txt').write_text(hashlib.sha256(archive.read_bytes()).hexdigest()+'  '+archive.name+'\n',encoding='utf-8')
print(archive)
