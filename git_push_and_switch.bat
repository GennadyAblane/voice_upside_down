@echo off
echo Checking git status...
git status

echo.
echo Adding all changes...
git add -A

echo.
echo Committing changes...
git commit -m "Add Android build scripts and documentation for Qt 6.7.2"

echo.
echo Pushing to remote...
git push

echo.
echo Checking available branches...
git branch -a

echo.
echo Switching to master branch...
git checkout master

echo.
echo Done!

