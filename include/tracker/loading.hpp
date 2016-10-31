static int loading_next, loading_step;
static int loading_len = 40;

static void loading_title(const char*title) {
  printf("%s\n", title);
  loading_next = 0;
}

static void loading_bar(int a, int b) {
  if (a != loading_next) return;
  if (!loading_next) {
    loading_step = b >= 1000 ? b/1000 : 1;
  }
  loading_next += loading_step;
  printf("|");
  int dist = a*2LL*loading_len/b;
  for (int i = 0; i < dist>>1; i++)
    printf("=");
  if (dist&1)
    printf("-");
  for (int i = dist+1>>1; i < loading_len; i++)
    printf(" ");
  printf("| %d / %d (%d%%)\r", a, b, int(a*100LL/b));
}

static void loading_finish(int b) {
  loading_next = b;
  loading_bar(b, b);
  printf("\n");
}
