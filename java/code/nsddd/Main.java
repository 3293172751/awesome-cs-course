/*
 * @Description: file 
 * @Author: xiongxinwei 3293172751nss@gmail.com
 * @Date: 2022-09-11 17:00:59
 * @LastEditTime: 2022-09-11 17:07:33
 * @FilePath: \code\nsddd\Main.java
 * @blog: https://nsddd.top
 */
import java.io.File;  // Import the File class
import java.io.IOException;  // Import the IOException class to handle errors
import java.io.IOException;  // Import the IOException class to handle errors

public class Main {
  public static void main(String[] args) {
    try {
      File myObj = new File("filename.txt");  //����һ���ļ�
      if (myObj.createNewFile()) {  //��������ɹ�
        System.out.println("File created: " + myObj.getName());  //��ȡ�ļ�����
      } else {
        System.out.println("File already exists.");
      }
    } catch (IOException e) {   //����ʧ��
      System.out.println("An error occurred.");
      e.printStackTrace();
    }

    //д���ļ�
    try {
        //FileWriter myObj = new FileWriter("filename.txt");
        myObj.write("Files in Java might be tricky, but it is fun enough!");
        myObj.close();
        System.out.println("Successfully wrote to the file.");
      } catch (IOException e) {
        System.out.println("An error occurred.");
        e.printStackTrace();
      }
  }
}