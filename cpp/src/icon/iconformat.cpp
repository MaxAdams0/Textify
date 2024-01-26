// Stands for .ico/icon auto format, used for automatic .rc file path corrective
#include <string>
using std::string;
#include <vector>
using std::vector;
#include <fstream>
using std::ofstream;
#include <filesystem>
namespace fs = std::filesystem;
using std::filesystem::current_path;

vector<string> content;

string FindFile(string filetype)
{
  for (const auto & file : fs::directory_iterator(/*path*/current_path().string())){
    content.push_back(file.path().string());
  }

  for (string i : content){
    if (i.find(filetype) != string::npos){
      return i;
    }
  }

  return "ERROR: cannot find .rc file";
}

int main()
{
  if (FindFile(".rc").find("ERROR") == string::npos){
    ofstream rc_file;
    rc_file.open("iconadd.rc");
    rc_file << "id ICON \"" << FindFile(".ico") << "\"";
    rc_file.close();
  } else {
    ofstream err_file;
    err_file.open("RCLOADFAIL.txt");
    err_file << "You really didn't want't to see this, did you?";
    err_file.close();
  }
}