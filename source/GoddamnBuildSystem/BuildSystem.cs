﻿using System;
using System.IO;

namespace GoddamnEngine.BuildSystem
{
    public sealed class BuildSystem
    {
        private static string _SDKPath = null;
        public static string SDKPath 
        { 
            get 
            {
                if (BuildSystem._SDKPath == null)
                {
                    BuildSystem._SDKPath = Environment.GetEnvironmentVariable("GODDAMN_SDK");
                    if (BuildSystem._SDKPath == null)
                        BuildSystem._SDKPath = Path.Combine(Path.Combine(Environment.CurrentDirectory, ".."), "..") + Path.DirectorySeparatorChar;
                }

                return BuildSystem._SDKPath;
            } 
        }

        static void Main(string[] args)
        {
            string SolutionLocation = @"D:\GoddamnEngine\source";
            Project.ProcessProjectsInDirectory(SolutionLocation);

        //  foreach (var ProjectFile in Directory.EnumerateFiles(BuildSystem.SDKPath, "*.gdproj.cs", SearchOption.AllDirectories))
        //      new Thread(() => new VisualStudioTarget().GenerateProject(SolutionProject.CreateProjectFromSource(ProjectFile))).Start();
        }
    }   // class BuildSystem
}   // namespace GoddamnEngine.BuildSystem
