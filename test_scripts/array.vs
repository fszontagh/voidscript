string[] $array = ["cat", "dog", "girafe"];

int[] $intArray = [10,11,0,1,2,44,2];

int[] $emptyIntArray = [];



printnl("First key: ", $array[0]);

int index = 0;
for (string $value : $array) {
    printnl("$value =",$value, " ?= $array[$index] =", $array[$index]);
    $index = $index + 1;
}


int $size = sizeof($array);

printnl("The size of the $array: ", $size);