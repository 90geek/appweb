/*
    load.tst - Load tests
 */

const HOST = (global.tsession && tsession["host"]) || "http://127.0.0.1:4100"

if (test.depth >= 4) {
    let command = Cmd.locate("testAppweb").portable + " --host " + HOST + 
        " --name mpr.api.c --iterations 400 " + test.mapVerbosity(-2)

    Cmd.sh(command)
    if (test.multithread) {
        Cmd.sh(command + " --threads " + 2)
    }
    if (test.multithread) {
        for each (count in [2, 4, 8, 16]) {
            Cmd.sh(command + " --threads " + count)
        }
    }
} else {
    test.skip("Runs at depth 4")
}
