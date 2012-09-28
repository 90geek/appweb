/*
    form.tst - Form-based authentication tests
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"
const HTTPS = App.config.uris.https || "127.0.0.1:4110"

let http: Http = new Http

if (App.config.bit_ssl) {
    //  Will be denied
    // http.setCredentials("anybody", "wrong password")
    http.get(HTTP + "/auth/form/index.html")
    assert(http.status == 302)
    let location = http.header('location')
    assert(location.contains('https'))
    assert(location.contains('login.esp'))

    //  Will return login form
    http.get(location)
    assert(http.status == 200)
    assert(http.response.contains("<form"))
    assert(http.response.contains('action="/auth/form/login"'))

    //  Login. The response should redirct to /auth/form
    http.reset()
    // http.setCookie(cookie)
    http.form("https://" + HTTPS + "/auth/form/login", {username: "joshua", password: "pass1"})
    assert(http.status == 302)
    location = http.header('location')
    assert(location.contains('https'))
    assert(location.contains('/auth/form'))
    let cookie = http.header("Set-Cookie")
    assert(cookie.match(/(-http-session-=.*);/)[1])

    //  Now logged in
    http.reset()
    http.setCookie(cookie)
    http.get("https://" + HTTPS + "/auth/form/index.html")
    assert(http.status == 200)

    //  Now log out. Will be redirected to the login page.
    http.reset()
    http.setCookie(cookie)
    http.get("https://" + HTTPS + "/auth/form/logout")
    assert(http.status == 302)
    let location = http.header('location')
    assert(location.contains('https'))
    assert(location.contains('login.esp'))

    //  Should fail to access index.html and get the login page again.
    http.get(HTTP + "/auth/form/index.html")
    assert(http.status == 302)
    let location = http.header('location')
    assert(location.contains('https'))
    assert(location.contains('login.esp'))

    http.close()
} else {
    test.skip("SSL not enabled")
}

