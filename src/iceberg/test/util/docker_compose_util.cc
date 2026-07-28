/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "iceberg/test/util/docker_compose_util.h"

#include <chrono>
#include <print>
#include <stdexcept>
#include <thread>

#include "iceberg/test/util/cmd_util.h"

namespace iceberg {

namespace {

// Compose pulls the fixture image from Docker Hub on every run, and that request
// intermittently times out on CI runners.
constexpr int kUpMaxAttempts = 3;
constexpr std::chrono::seconds kUpRetryBackoff{5};

}  // namespace

DockerCompose::DockerCompose(std::string project_name,
                             std::filesystem::path docker_compose_dir)
    : project_name_(std::move(project_name)),
      docker_compose_dir_(std::move(docker_compose_dir)) {}

DockerCompose::~DockerCompose() { Down(); }

void DockerCompose::Up() {
  for (int attempt = 1;; ++attempt) {
    try {
      auto cmd = BuildDockerCommand({"up", "-d", "--wait", "--timeout", "60"});
      cmd.RunCommand("docker compose up");
      return;
    } catch (const std::runtime_error&) {
      if (attempt >= kUpMaxAttempts) {
        throw;
      }
      std::println(stderr, "[INFO] docker compose up attempt {}/{} failed, retrying",
                   attempt, kUpMaxAttempts);
      try {
        Down();
      } catch (const std::runtime_error&) {
        // Nothing worth reporting: the retry re-runs up either way, and the
        // original failure is what surfaces if attempts run out.
      }
      std::this_thread::sleep_for(kUpRetryBackoff * attempt);
    }
  }
}

void DockerCompose::Down() {
  auto cmd = BuildDockerCommand({"down", "-v", "--remove-orphans"});
  return cmd.RunCommand("docker compose down");
}

Command DockerCompose::BuildDockerCommand(const std::vector<std::string>& args) const {
  Command cmd("docker");
  // Set working directory
  cmd.CurrentDir(docker_compose_dir_);
  // Use 'docker compose' subcommand with project name
  cmd.Arg("compose").Arg("-p").Arg(project_name_).Args(args);
  return cmd;
}

}  // namespace iceberg
