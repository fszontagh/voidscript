int $num = 123;
double $double = 12.3;
string $variable = "This is a string content with a number: 123";

string $variable2 = $variable;

function test = (int $i) {
    print("Param: ",$i);
    int $result = $i + 1;
}

function increment = (int $i) int {
    return $i + 1;
}


int $z = 10;
increment($z);
int $t = increment(2);
print("The result is: ", $t);