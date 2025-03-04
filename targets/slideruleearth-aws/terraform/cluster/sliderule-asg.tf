resource "aws_autoscaling_group" "sliderule-cluster" {
  launch_configuration  = aws_launch_configuration.sliderule-instance.id
  desired_capacity      = var.node_asg_desired_capacity
  min_size              = var.node_asg_min_capacity
  max_size              = var.node_asg_max_capacity
  health_check_type     = "EC2"
  vpc_zone_identifier   = [ aws_subnet.sliderule-subnet.id ]
  tag {
    key                 = "Name"
    value               = "${var.cluster_name}-node"
    propagate_at_launch = true
  }
}

resource "aws_launch_configuration" "sliderule-instance" {
  image_id                    = data.aws_ami.sliderule_cluster_ami.id
  instance_type               = "r6g.xlarge"
  root_block_device {
    volume_type               = "gp2"
    volume_size               = 8
    delete_on_termination     = true
  }
  key_name                    = var.key_pair_name
  associate_public_ip_address = true
  security_groups             = [aws_security_group.sliderule-sg.id]
  iam_instance_profile        = aws_iam_instance_profile.s3-role.name
  user_data = <<-EOF
      #!/bin/bash
      export ENVIRONMENT_VERSION=${var.environment_version}
      export IPV4=$(hostname -I | awk '{print $1}')
      aws ecr get-login-password --region us-west-2 | docker login --username AWS --password-stdin 742127912612.dkr.ecr.us-west-2.amazonaws.com
      export CLUSTER=${var.cluster_name}
      export SLIDERULE_IMAGE=${var.container_repo}/sliderule:${var.cluster_version}
      export PROVISIONING_SYSTEM="https://ps.${var.domain}"
      aws s3 cp s3://sliderule/infrastructure/software/${var.cluster_name}-docker-compose-sliderule.yml ./docker-compose.yml
      docker-compose -p cluster up --detach
  EOF
}
