public class Main {
	/**
	 * @param args
	 */
	public static void main(String[] args) {
		Person p1 = new Person();
        p1.name = "milan";
        p1.age = 14;
        
        MyTools tools = new MyTools();
        Person p2 = tools.copyPerson(p1);
        
        // ����p������
        System.out.println("p1������name =" + p1.name + "p1.age=" + p1.age);
        System.out.println("p2������name =" + p2.name + "p2.age=" + p2.age);
        //�ж��Ƿ����
        System.out.println(p1 == p2);
        //�޸�p2����

    }
}

class Person {
    String name;
    int age;
}

class MyTools{
    public Person copyPerson(Person p1) {
        //1. ע�⺯���ķ������ͺʹ������� -- ��Person
        //2. ����������
        //3. �������β� --- ���Ը�ֵPerson���βξ���Person
        //4. ������ -- ����һ���¶��󣬲��Ҹ�ֵ���Լ���
        Person p2 = new Person();
        p2.name = p1.name;  //ԭ���������ָ�ֵ��
        p2.age = p1.age;
    	return p2;  
    }
}

