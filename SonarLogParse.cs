/// Read well water depth log file, extract depth and time/date values
/// decimate the data rate, and write to output file
/// using C# 10.0, .NET 6   J.Beale Jan 2022

using static System.Console;
using static System.IO.Path;
using static System.Environment;


ReadSonarLog();

static void ReadSonarLog()
{   // fileIn : data file from well depth logger

    string fileIn = "rp51-log5.csv";
    string pathIn = Combine("M:", fileIn);
    WriteLine($"Now reading {pathIn}");

    string dirOut = Combine( GetFolderPath(SpecialFolder.Personal), "sonar");
    string fileOut = "rp51_out.csv";
    string pathOut = Combine(dirOut, fileOut);

    int dRate = 10;  // decimation ratio

    using (StreamWriter sw = new(pathOut))
    {
        sw.WriteLine("date, epoch, meters");
        int lineCount = 1;
        string outString = "";  // one line of processed output
        string outFirst = "";   // first line of processed output
        foreach (string oneLine in File.ReadLines(pathIn))
        {   // ignore the header line, and comment lines
            if ((lineCount != 1) && (oneLine[0] != '#'))
            {
                string[] elems = oneLine.Split(' ');
                if (lineCount % dRate == 0)
                {
                    try { 
                        long epoch = int.Parse(elems[0]);
                        DateTimeOffset date =
                            DateTimeOffset.FromUnixTimeSeconds(epoch).ToLocalTime();
                        outString = $"{date.ToString("yyyy-MM-dd_HHmmss")},{epoch},-{elems[2]}";                    
                        sw.WriteLine(outString);
                        if (outFirst == "")
                        {
                            outFirst = outString;
                        }
                    }
                    catch (System.FormatException e)
                    {
                        WriteLine($"Error at {lineCount}: {oneLine}");
                    }
                }
            }
            lineCount++;
        } // foreach (line in input file)
        WriteLine($"Start: {outFirst}");
        WriteLine($"End:   {outString}");
    }
}
