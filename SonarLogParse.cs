/// Read well water depth log file, extract depth and time/date values
/// decimate the data rate, and write to output file
/// using C# 10.0, .NET 6   J.Beale Jan 2022

namespace LogProcess;  // file-scoped namespace

using static System.Console;
using static System.IO.Path;
using static System.Environment;

public class Program
{
    public static void Main(string[] args)
    {
        int argNum = args.Length;
        if (argNum != 2)
        {
            WriteLine("LogProcess needs input and output filenames, for example:");
            WriteLine(@"ProcFile1.exe M:\rp51_log5.csv C:\Users\beale\Documents\sonar\rp51_out.csv");
        }
        else
        {
            ReadSonarLog(args[0],args[1]);
        }
    }

    /// <summary>
    /// Read data file from well depth logger, and write out select columns
    /// </summary>
    /// <param name="pathIn">Pathname of input file</param>
    /// <param name="pathOut">Pathname of output file</param>
    static void ReadSonarLog(string pathIn, string pathOut)
    {   
        //string fileIn = "rp51_log5.csv";
        //string pathIn = Combine("M:", fileIn);
        //string dirOut = Combine(GetFolderPath(SpecialFolder.Personal), "sonar");
        //string fileOut = "rp51_out.csv";
        //string pathOut = Combine(dirOut, fileOut);

        WriteLine($"Now reading {pathIn}");
        WriteLine($"Output to {pathOut}");

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
                        try
                        {
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
            WriteLine($"Total line count: {lineCount}");
        }
    }

}
