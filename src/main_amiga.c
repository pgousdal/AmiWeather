int amiweather_gui_run(void);

static const char stack_cookie[] __attribute__((used)) = "$STACK:32768";

int main(void)
{
    return amiweather_gui_run();
}
