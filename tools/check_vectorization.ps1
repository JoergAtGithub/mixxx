param($compiler, $flags, $file)

# Compile the code with vectorization enabled and save the compiler's output
& $compiler $flags $file > output.txt

# Extract the names of all functions in the file
$functions = Select-String -Path $file -Pattern '(\w+)\s*\([^)]*\)\s*\{' | % { $_.Matches } | % { $_.Groups[1].Value }

# Check the output for each function
foreach ($function in $functions) {
    if (Select-String -Path output.txt -Pattern "$function.*loop vectorized") {
        Write-Output "$($env:OS), $compiler, $function, Yes"
    } else {
        Write-Output "$($env:OS), $compiler, $function, No"
    }
}
