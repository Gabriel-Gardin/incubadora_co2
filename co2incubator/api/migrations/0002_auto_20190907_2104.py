# Generated by Django 2.2.1 on 2019-09-07 21:04

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('api', '0001_initial'),
    ]

    operations = [
        migrations.AlterField(
            model_name='estufa1_data',
            name='co2',
            field=models.FloatField(max_length=200),
        ),
        migrations.AlterField(
            model_name='estufa1_data',
            name='temperatura',
            field=models.FloatField(),
        ),
    ]