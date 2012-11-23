# MDILDump

This is a dump tool to parse the .mdil section in .NET assemblies containing MDIL or "Machine Dependent Intermediate Language".

This work is completely for personal research purposes.

Currently it only supports assemblies generated by CrossGen 4.0.50829.0 in [Windows Phone 8 SDK RTM](https://www.microsoft.com/download/details.aspx?id=35471).  
Usually it will be installed in __%ProgramFiles(x86)%\Microsoft SDKs\Windows Phone\\v8.0\Tools\MDILXAPCompile__

System assemblies containing ARM MDIL code are installed in  
**%ProgramFiles(x86)%\Microsoft SDKs\Windows Phone\\v8.0\DeviceNativeLibraries**

System assemblies in the WP8 emulation VHD images contain x86 MDIL code, valuable for comparision  
__%ProgramFiles(x86)%\Microsoft SDKs\Windows Phone\\v8.0\Emulation\Images\Flash.vhd__  
__[SystemPartition]\Windows\System32\\\*.ni.dll__

# About MDIL

Microsft introduced MDIL for CoreCLR with the release of Windows Phone 8 and its SDK.  
See [https://channel9.msdn.com/Shows/Going+Deep/Mani-Ramaswamy-and-Peter-Sollich-Inside-Compiler-in-the-Cloud-and-MDIL](https://channel9.msdn.com/Shows/Going+Deep/Mani-Ramaswamy-and-Peter-Sollich-Inside-Compiler-in-the-Cloud-and-MDIL)

Some of the technical specifications of MDIL can be found in these patents:  
**Intermediate Language Support For Change Resilience**  
US 20110258615 [http://www.freepatentsonline.com/y2011/0258615.html](http://www.freepatentsonline.com/y2011/0258615.html) or  
US 20110258616 [http://www.freepatentsonline.com/y2011/0258616.html](http://www.freepatentsonline.com/y2011/0258616.html)  
The information in these documents are a little bit outdated but still very useful and accurate, MDILDump is mainly based on this.

The Bartok compiler in [Singularity RDK](https://singularity.codeplex.com/) and Verve OS (the "verify" folder) also supports MDIL but very old and outdated.