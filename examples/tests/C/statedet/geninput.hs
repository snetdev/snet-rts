
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
  let rs = (\(a,g) -> a : rs g) . randomR (lower, upper)
  fmap rs newStdGen

main = do
  (num : max : _) <- fmap ((++) [10, 10] . map read) getArgs
  myInts <- fmap (take num) $ randomInts 0 max
  let mySum = sum myInts
  putStr header
  putStr $ stateRecord mySum
  mapM_ (putStr . invalRecord) myInts
  putStr footer


