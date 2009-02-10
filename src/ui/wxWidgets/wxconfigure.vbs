' Simple VBScript to set up the Visual Studio Properties file for PasswordSafe

Dim objFileSystem, objOutputFile
Dim strOutputFile, strFileLocation, strWXWIN, strCurrentWXWIN
Dim str1, str2, str3,CRLF
Dim rc

CRLF = Chr(13) & Chr(10)
const strTortoiseSVNDir = "C:\Program Files\TortoiseSVN"
const strXercesDir = "C:\Program Files\xerces-c-3.0.0-x86-windows-vc-8.0"
const strExpatDir = "C:\Program Files\Expat 2.0.1"
const strMSXML60SDKDir = "C:\Program Files\MSXML 6.0"

Set objShell = WScript.CreateObject("WScript.Shell")
Set wshUserEnv = objShell.Environment("USER")
strCurrentWXWIN = wshUserEnv("WXWIN")

Set wshUserEnv = Nothing
Set objShell = Nothing

If (Len(strCurrentWXWIN) = 0) Then
  strWXWIN = "C:\wxWidgets-2.8.9"
Else
  strWXWIN = WXWIN
End If

str1 = "Please supply fully qualified location, without quotes, where "
str2 = " was installed." & CRLF & "Leave empty or pressing Cancel for default to:" & CRLF & CRLF
str3 = CRLF & CRLF & "See README.DEVELOPERS.txt for more information."

strOutputFile = "wxUserVariables.vsprops"

Set objFileSystem = CreateObject("Scripting.fileSystemObject")

If (objFileSystem.FileExists(strOutputFile)) Then
  ' vbYesNo | vbQuestion | vbDefaultButton2 = 4 + 32 + 256 = 292
  rc = MsgBox("File """ & strOutputFile & """ already exists! OK to overwrite?", 292)
  ' vbNo = 7
  If (rc = 7) Then
    Set objFileSystem = Nothing
    WScript.Quit(0)
  End If
End If

Set objOutputFile = objFileSystem.CreateTextFile(strOutputFile, TRUE)

objOutputFile.WriteLine("<?xml version=""1.0"" encoding=""Windows-1252""?>")
objOutputFile.WriteLine("<VisualStudioPropertySheet")
objOutputFile.WriteLine("  ProjectType=""Visual C++""")
objOutputFile.WriteLine("	Version=""8.00""")
objOutputFile.WriteLine("	Name=""UserVariables""")
objOutputFile.WriteLine("	>")
objOutputFile.WriteLine("	<UserMacro")
objOutputFile.WriteLine("		Name=""ProjectDir""")
objOutputFile.WriteLine("		Value=""&quot;$(ProjectDir)&quot;""")
objOutputFile.WriteLine("		PerformEnvironmentSet=""true""")
objOutputFile.WriteLine("	/>")
objOutputFile.WriteLine("	<UserMacro")
objOutputFile.WriteLine("		Name=""TortoiseSVNDir""")
strFileLocation = InputBox(str1 & "Tortoise SVN" & str2 & strTortoiseSVNDir & str3, "Tortoise SVN Location", strTortoiseSVNDir)
If (Len(strFileLocation) = 0) Then strFileLocation = strTortoiseSVNDir
objOutputFile.WriteLine("		Value=""" & strFileLocation & """")
objOutputFile.WriteLine("		PerformEnvironmentSet=""true""")
objOutputFile.WriteLine("	/>")
objOutputFile.WriteLine("	<UserMacro")
objOutputFile.WriteLine("		Name=""ExpatDir""")
strFileLocation = InputBox(str1 & "Expat" & str2 & strExpatDir &str3 , "Expat Location", strExpatDir)
If (Len(strFileLocation) = 0) Then strFileLocation = strExpatDir
objOutputFile.WriteLine("		Value=""" & strFileLocation & """")
objOutputFile.WriteLine("		PerformEnvironmentSet=""true""")
objOutputFile.WriteLine("	/>")
objOutputFile.WriteLine("	<UserMacro")
objOutputFile.WriteLine("		Name=""MSXML60SDKDir""")
strFileLocation = InputBox(str1 & "MS XML6 SDK" & str2 & strMSXML60SDKDir &str3, "MS XML6 SDK Location", strMSXML60SDKDir)
If (Len(strFileLocation) = 0) Then strFileLocation = strMSXML60SDKDir
objOutputFile.WriteLine("		Value=""" & strFileLocation & """")
objOutputFile.WriteLine("		PerformEnvironmentSet=""true""")
objOutputFile.WriteLine("	/>")
objOutputFile.WriteLine("	<UserMacro")
objOutputFile.WriteLine("		Name=""XercesDir""")
strFileLocation = InputBox(str1 & "Xerces" & str2 & strXercesDir & str3, "Xerces Location", strXercesDir)
If (Len(strFileLocation) = 0) Then strFileLocation = strXercesDir
objOutputFile.WriteLine("		Value=""" & strFileLocation & """")
objOutputFile.WriteLine("		PerformEnvironmentSet=""true""")
objOutputFile.WriteLine("	/>")
objOutputFile.WriteLine("	<UserMacro")
objOutputFile.WriteLine("		Name=""WXWIN""")
strFileLocation = InputBox(str1 & "wxWidgets" & str2 & strWXWIN &str3, "wxWidgets installation directory", strWXWIN)
If (Len(strFileLocation) = 0) Then strFileLocation = strWXWIN
objOutputFile.WriteLine("		Value=""" & strFileLocation & """")
objOutputFile.WriteLine("		PerformEnvironmentSet=""true""")
objOutputFile.WriteLine("	/>")
objOutputFile.WriteLine("</VisualStudioPropertySheet>")

objOutputFile.Close

Set objFileSystem = Nothing

WScript.Quit(0)
