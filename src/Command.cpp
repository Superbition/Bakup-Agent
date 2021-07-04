#include "Command.h"

Command::Command(Debug &debug, string shell) : debug(ref(debug)), shell(std::move(shell))  {}

Command::~Command()
{
    // Close the write end of the write pipe
    close(this->outPipeFD[1]);

    // Close the read end of the read pipe
    close(this->inPipeFD[0]);
}

string Command::generateDelimiter()
{
    thread_local std::mt19937 prng(std::random_device{}());
    thread_local std::uniform_int_distribution dist('a', 'z');
    thread_local auto gen = [&]() { return dist(prng); };

    std::string delimiter(128, 0);
    std::generate(delimiter.begin(), delimiter.end(), gen);

    return delimiter;
}

/*
 * Must only be ran once per Command object.
 * This will setup a child process and establish the pipes for reading and writing to it
 */
bool Command::setupEnvironment(string bashTestCommand)
{
    // Create a read and write pipe for communication with the child process
    pipe(this->inPipeFD);
    pipe(this->outPipeFD);

    // Set read pipes to non-blocking and handle waiting in the runCommand method
    fcntl(this->inPipeFD[0], F_SETFL, fcntl(this->inPipeFD[0], F_GETFL) & O_NONBLOCK);
    fcntl(this->inPipeFD[1], F_SETFL, fcntl(this->inPipeFD[1], F_GETFL) & O_NONBLOCK);

    // Create a child to run the job commands in
    this->pid = fork();

    // Check child was made
    if(this->pid < 0)
    {
        // Print an error to console
        this->debug.error("Could not open a child process to run the job");

        // Return error to caller
        return false;
    }
    else if(this->pid == 0)
    {
        // Close STDIN and replace it with outPipeFD read end
        dup2(this->outPipeFD[0], STDIN_FILENO);
        close(this->outPipeFD[0]); // not needed anymore

        // Close STDOUT and replace it with inPipe read end
        dup2(this->inPipeFD[1], STDOUT_FILENO);
        close(this->inPipeFD[1]); // not needed anymore

        // Start a bash shell and echo success message
        execl(this->shell.c_str(), nullptr);

        // Make child kill itself if execl fails
        kill(getpid(), SIGTERM);
        return false;
    }
    else
    {
        // Close the read end of the write pipe
        close(this->outPipeFD[0]);

        // Close the write end of the read pipe
        close(this->inPipeFD[1]);
    }

    // For convenience and ease of maintainability, set the parent's in and out pipes
    this->out = this->outPipeFD[1];
    this->in = this->inPipeFD[0];

    // Wait for the child to change state
    int status;
    pid_t result = waitpid(this->pid, &status, WNOHANG);
    if (result == 0) // If the child is alive
    {
        // Check that the bash shell is setup correctly by running a command
        auto [output, exitStatus] = runCommand(bashTestCommand);
        if(exitStatus != EXIT_SUCCESS) // If failed
        {
            return false;
        }
        else // Else, successful
        {
            return true;
        }
    }
    else if (result == -1) // Else, there was an error returned by waitpid
    {
        return false;
    }
    else // Else, the child died
    {
        return false;
    }
}

std::pair<string, exit_status_t> Command::runCommand(string cmd)
{
    // Buffer size for things being sent through pipes
    constexpr size_t BufSize = 1024;

    // a string that is unlikely to show up in the output:
    const std::string delimiter = generateDelimiter() + "\n";

    // Add an echo of the status of the command that was ran
    cmd += " 2>&1\necho -e $?\"\\n\"" + delimiter;

    // Send the command to the child process
    auto len = write(this->out, cmd.c_str(), cmd.size());

    // If the length is 0 or less, the pipe couldn't be written to
    if(len <= 0)
    {
        return {{}, ES_WRITE_FAILED};
    }

    // Wipe command and use it to collect all read data
    cmd.resize(0);

    // Use buffer to store each line being read
    std::string buffer(BufSize, 0);

    // Loop and read all data until the delimiter is found
    fd_set read_set{};
    FD_SET(this->in, &read_set);
    while(true) {
        // Wait until something happens on the pipe
        select(this->in + 1, &read_set, nullptr, nullptr, nullptr);

        // If the read failed on the pipe
        if((len = read(this->in, buffer.data(), buffer.size())) <= 0)
        {
            // Set an error code and break reading
            cmd += "\n" + std::to_string(ES_READ_FAILED) + "\n" + delimiter;
            break;
        }

        // Append what was read to cmd
        cmd.append(buffer.begin(), buffer.begin() + len);

        // If the delimiter was found, break
        if(cmd.size() >= delimiter.size() && cmd.substr(cmd.size() - delimiter.size()) == delimiter)
        {
            break;
        }
    }

    // Remove the delimiter from the command output and trailing newline
    cmd.resize(cmd.size() - (delimiter.size() + 1));

    // Get the position of the newline that is in front of the exit status
    int newLinePosition = cmd.rfind('\n');

    // Extract the exit status and convert to int
    exit_status_t exitStatus = static_cast<exit_status_t>(stoi(cmd.substr(newLinePosition + 1, cmd.size())));

    // Remove the exit status from the command output
    string output = cmd.substr(0, newLinePosition);

    // Return the pair
    return {output, exitStatus};
}

bool Command::setShell(string &shell) {
    this->shell = shell;
}

pid_t Command::getChildPid()
{
    return this->pid;
}

void Command::killChild()
{
    // Check that the child was started correctly and that a pid was set
    if(this->pid > -1)
    {
        // Kill the child
        kill(this->pid, SIGKILL);
    }
}