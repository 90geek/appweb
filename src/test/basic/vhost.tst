/*
    vhost.tst - Virtual Host tests
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"
const NAMED = App.config.uris.named || "127.0.0.1:4111"
const VIRT = App.config.uris.virt || "127.0.0.1:4112"

let http: Http = new Http

function mainHost() {
    http.get(HTTP + "/main.html")
    http.response.contains("HTTP SERVER")
    assert(http.status == 200)

    //  These should fail
    http.get(HTTP + "/iphost.html")
    assert(http.status == 404)
    http.get(HTTP + "/vhost1.html")
    assert(http.status == 404)
    http.get(HTTP + "/vhost2.html")
    assert(http.status == 404)
}


//  The first virtual host listens to "localhost", the second listens to HTTP.

function namedHost() {
    http = new Http
    http.setHeader("Host", "localhost:" + Uri(NAMED).port)
    http.get(NAMED + "/vhost1.html")
    assert(http.status == 200)

    //  These should fail
    http = new Http
    http.get(NAMED + "/main.html")
    assert(http.status == 404)
    http.get(NAMED + "/iphost.html")
    assert(http.status == 404)
    http.get(NAMED + "/vhost2.html")
    assert(http.status == 404)

    //  Now try the second vhost on 127.0.0.1
    http = new Http
    http.setHeader("Host", "127.0.0.1:" + Uri(NAMED).port)
    http.get(NAMED + "/vhost2.html")
    assert(http.status == 200)

    //  These should fail
    http.get(NAMED + "/main.html")
    assert(http.status == 404)
    http.get(NAMED + "/iphost.html")
    assert(http.status == 404)
    http.get(NAMED + "/vhost1.html")
    assert(http.status == 404)
}

function ipHost() {
    let http: Http = new Http
    http.setCredentials("mary", "pass2")
    http.get(VIRT + "/private.html")
    assert(http.status == 200)
}

mainHost()
namedHost()
ipHost()
