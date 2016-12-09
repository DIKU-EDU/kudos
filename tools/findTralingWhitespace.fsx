open System.IO

// directories to search through, add more directories below
let dirs = ["../kudos"]

let rootDir = Directory.GetCurrentDirectory()
let dirNames = List.map (fun x -> Path.Combine(rootDir,x)) dirs

let dirNotFound(path: string) =
    printfn "'%s' could not be found" path


let rec findFiles = function
    | ([],f) -> f
    | (d::ds,f) ->
        findFiles(ds @ List.ofArray(Directory.GetDirectories(d)),
                  f @ List.ofArray(Directory.GetFiles(d)))


let searchFile(path: string) =
    let mutable tCount = 0
    let fstream = File.Open(path, FileMode.Open, FileAccess.Read)
    let mutable byte = fstream.ReadByte()
    let mutable spc = 0
    while byte <> -1 do
        match byte with
            | 32 -> spc <- spc + 1
            | 10 -> tCount <- tCount + spc; spc <- 0
            | _  -> spc <- 0
        byte <- fstream.ReadByte()
    if tCount > 0 then
        printfn "\n%i trailing whitespaces where found in '%s'"
            tCount path
    fstream.Close()
    tCount


let main() =
    let mutable isOkay = true
    let mutable hasBeenFound = false

    for dirname in dirNames do
        if Directory.Exists(dirname) |> not then
            dirNotFound(dirname)
            isOkay <- false

    let collection = if isOkay then findFiles(dirNames, []) else []

    for filename in collection do
        if File.Exists(filename) then
            if searchFile(filename) > 0 then
                hasBeenFound <- true
        else dirNotFound(filename)

    if not hasBeenFound then
        printfn "No trailing whitespaces have been found."

main()
