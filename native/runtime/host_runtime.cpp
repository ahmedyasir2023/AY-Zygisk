// AY Zygisk Runtime v0.3.0 - host-only isolated plugin lifecycle laboratory.
// NOT a Zygote injector or production loader. Never built/shipped on Android.
// Copyright (c) 2026 Eng. Ahmed Yasir. SPDX-License-Identifier: MIT.
#include "ay_runtime_api.h"
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {
std::string scrub(const char *s, size_t limit = 128) {
  if (!s) return "null";
  std::string o;
  for (size_t i = 0; s[i] && i < limit; ++i) {
    unsigned char c = static_cast<unsigned char>(s[i]);
    o += (c >= 32 && c < 127 && c != '|') ? static_cast<char>(c) : '_';
  }
  return o;
}
void send_line(int fd, const std::string &s) {
  // Structured IPC is bounded and does not log arbitrary plugin output.
  const auto n = std::min(s.size(), static_cast<size_t>(512));
  (void)write(fd, s.data(), n);
}
void worker(int fd, const char *path) {
  struct rlimit core = {0, 0};
  (void)setrlimit(RLIMIT_CORE, &core);
  struct rlimit cpu = {3, 3};
  (void)setrlimit(RLIMIT_CPU, &cpu);
  struct rlimit mem = {512ULL*1024*1024, 512ULL*1024*1024};
  (void)setrlimit(RLIMIT_AS, &mem);
  // Exercise the *AY-only* test interface. Do not call arbitrary Zygisk ABI symbols.
  void *h = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if (!h) { send_line(fd, "FAIL|dlopen-error\n"); _exit(30); }
  void *p = dlsym(h, "ay_runtime_get_v1");
  if (!p) { send_line(fd, "FAIL|missing-ay-entry\n"); dlclose(h); _exit(31); }
  auto get = reinterpret_cast<ay_runtime_get_plugin_v1>(p);
  const ay_runtime_plugin_v1 *plugin = get();
  if (!plugin || plugin->abi_version != AY_RUNTIME_ABI_V1 || !plugin->on_event || !plugin->id) {
    send_line(fd, "FAIL|unsupported-ay-abi\n"); dlclose(h); _exit(32);
  }
  char out[128]{};
  if (plugin->on_event(AY_RUNTIME_PREPARE, "simulated-test", out, sizeof(out)) != 0) {
    send_line(fd, "FAIL|prepare-callback\n"); dlclose(h); _exit(33);
  }
  char end[128]{};
  if (plugin->on_event(AY_RUNTIME_COMPLETE, "simulated-test", end, sizeof(end)) != 0) {
    send_line(fd, "FAIL|complete-callback\n"); dlclose(h); _exit(34);
  }
  send_line(fd, "OK|" + scrub(plugin->id) + "|prepare|" + scrub(out) + "|complete|" + scrub(end) + "\n");
  dlclose(h);
  _exit(0);
}
int run(const char *path) {
  if (geteuid() == 0) { std::cerr << "Refusing privileged runtime test; use an unprivileged account.\n"; return 7; }
  struct stat st{};
  if (lstat(path, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 64 || st.st_size > 32*1024*1024) {
    std::cout << "FAIL|invalid-plugin-file\n";return 2;
  }
  if (st.st_mode & (S_IWGRP | S_IWOTH)) {std::cout<<"FAIL|insecure-file-permissions\n";return 2;}
  int fds[2];
  if (pipe(fds) != 0) { std::cout << "FAIL|pipe\n";return 2; }
  pid_t pid=fork();
  if (pid < 0) { close(fds[0]);close(fds[1]); std::cout << "FAIL|fork\n"; return 2; }
  if (!pid) { close(fds[0]); worker(fds[1], path); }
  close(fds[1]);
  int status=0; bool completed=false;
  auto start=std::chrono::steady_clock::now();
  while (std::chrono::steady_clock::now()-start < std::chrono::milliseconds(2500)) {
    pid_t r=waitpid(pid, &status, WNOHANG);
    if (r==pid) { completed=true; break; }
    if (r<0) break;
    usleep(10000);
  }
  if (!completed) {
    kill(pid, SIGKILL); waitpid(pid, &status, 0);
    close(fds[0]); std::cout<<"FAIL|timeout-or-wait-error\n"; return 5;
  }
  char msg[1024]{};
  ssize_t n=read(fds[0], msg, sizeof(msg)-1);
  close(fds[0]);
  if (WIFSIGNALED(status)) {std::cout<<"FAIL|plugin-crashed|signal-"<<WTERMSIG(status)<<"\n";return 4;}
  if (n>0) std::cout.write(msg,n);
  else std::cout<<"FAIL|no-result\n";
  return WIFEXITED(status)&&WEXITSTATUS(status)==0 ? 0 : 3;
}
} // namespace
int main(int argc,char **argv) {
  if (argc==2 && std::string(argv[1])=="version") {
    std::cout<<"AY-RUNTIME|version|0.3.0\nAY-RUNTIME|environment|desktop-test-only\n";
    return 0;
  }
  if (argc==3 && std::string(argv[1])=="test-plugin") return run(argv[2]);
  std::cerr<<"Usage: ay-runtime-host version | test-plugin /path/to/AY-test.so\n";
  return 2;
}
