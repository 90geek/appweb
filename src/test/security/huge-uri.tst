/*
    Very large URI test (3MB)
 */ 
const HTTP = (global.tsession && tsession["http"]) || ":4100"
const port: Number = (global.tsession && tsession["port"]) || "4100"

//  This writes a 2MB URI
const ITER = 100

//  Data is 30K after this
let data = ""
for (i in 300) {
    data += "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678\n"
}

/*
    Test LimitUri
 */
let s = new Socket
s.connect(port)
let count = 0
try {
    count += s.write("GET ")
    for (i in ITER) {
        count += s.write(data)
    }
    count += s.write(" HTTP/1.1\r\n\r\n")
} catch {
    App.log.error("Write failed. Wrote  " + count + " of " + data.length * ITER + " bytes.")
    //  Check appweb.conf LimitRequestHeader. This must be sufficient to accept the write the header.
}

response = new ByteArray
while ((n = s.read(response, -1)) > 0) { }
// print(response)
assert(response.toString().contains("HTTP/1.1 414 Request-URI Too Large"))
s.close()

//  Check server still up
http = new Http
http.get(HTTP + "/index.html")
assert(http.status == 200)
http.close()
