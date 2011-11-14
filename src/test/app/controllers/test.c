/*
    Test mvc controller
 */
#include "esp.h"

static void common() {
    setParam("page-title", "MVC Title");
}

static void check() { 
    mprAssert(1 == 2);
    render("Check: OK\r\n");
    finalize();
    /* No view used */
}

static void details() { 
    setParam("secret", "42");
    /* View will be rendered */
}

static void login() { 
    if (getSessionVar("id")[0]) {
        render("Logged in");
        finalize();

    } else if (smatch(param("username"), "admin") && smatch(param("password"), "secret")) {
        render("Valid Login");
        finalize();
        setSessionVar("id", "admin");

    } else if (smatch(getMethod(), "POST")) {
        render("Invalid login, please retry.");
        finalize();

    } else {
        createSession();
    }
}

static void missing() {
    renderError(HTTP_CODE_INTERNAL_SERVER_ERROR, "Missing action");
}

ESP_EXPORT int esp_controller_test(HttpRoute *route, MprModule *module) {
    espDefineAction(route, "test-missing", missing);
    espDefineAction(route, "test-cmd-check", check);
    espDefineAction(route, "test-cmd-details", details);
    espDefineAction(route, "test-cmd-login", login);
    return 0;
}
