
import System.Environment (getArgs)
import System.Random (newStdGen, randomR)

header = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>\n"
footer = "<record type=\"terminate\" />\n"
record = "<record type=\"data\" mode=\"textual\" interface=\"C4SNet\">\n"
endtag = "</record>\n"
field label value =
  "  <field label=\"" ++ label ++ "\" >(int)" ++ show value ++ "</field>\n"

stateRecord num = record ++ field "state" num ++ endtag
invalRecord num = record ++ field "inval" num ++ endtag

randomInts :: Int -> Int -> IO [Int]
randomInts lower upper = do
  let rs g = let (a,g') = randomR (lower, upper) g in a : rs g'
  newStdGen >>= return . rs

main = do
  args <- getArgs
  let num = if null args then 10 else read $ head args
  let max = if length args < 2 then 10 else read $ args !! 1
  myInts <- randomInts 0 max >>= return . take num
  let mySum = sum myInts
  putStr header
  putStr $ stateRecord mySum
  mapM_ (putStr . invalRecord) myInts
  putStr footer


